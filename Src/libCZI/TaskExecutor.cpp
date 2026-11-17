// SPDX-FileCopyrightText: 2026 Carl Zeiss Microscopy GmbH
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TaskExecutor.h"

#include <algorithm>

using namespace libCZI::detail;

namespace
{
    thread_local bool is_task_executor_worker = false;
}

TaskExecutor& TaskExecutor::Get()
{
    static TaskExecutor executor;
    return executor;
}

TaskExecutor::TaskExecutor()
{
    const std::size_t worker_count = (std::min)(
        std::size_t{32},
        (std::max)(std::size_t{1}, static_cast<std::size_t>(std::thread::hardware_concurrency())));
    this->workers_.reserve(worker_count);
    for (std::size_t i = 0; i < worker_count; ++i)
    {
        this->workers_.emplace_back([this]() { this->WorkerLoop(); });
    }
}

TaskExecutor::~TaskExecutor()
{
    {
        std::lock_guard<std::mutex> lock(this->mutex_);
        this->stopping_ = true;
    }
    this->condition_.notify_all();
    for (auto& worker : this->workers_)
    {
        worker.join();
    }
}

bool TaskExecutor::IsWorkerThread()
{
    return is_task_executor_worker;
}

void TaskExecutor::WorkerLoop()
{
    is_task_executor_worker = true;
    for (;;)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(this->mutex_);
            this->condition_.wait(lock, [this]() { return this->stopping_ || !this->tasks_.empty(); });
            if (this->stopping_ && this->tasks_.empty())
            {
                break;
            }

            task = std::move(this->tasks_.front());
            this->tasks_.pop();
        }
        task();
    }
    is_task_executor_worker = false;
}
