// SPDX-FileCopyrightText: 2026 Carl Zeiss Microscopy GmbH
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace libCZI
{
    namespace detail
    {
        /// Process-wide bounded worker pool used for short read/decode jobs. Keeping the pool
        /// internal avoids changing the public API while preventing per-subblock thread creation.
        class TaskExecutor
        {
        public:
            static TaskExecutor& Get();

            template <typename TFunction>
            auto Submit(TFunction&& function)
                -> std::future<typename std::result_of<TFunction()>::type>
            {
                using Result = typename std::result_of<TFunction()>::type;
                auto task = std::make_shared<std::packaged_task<Result()>>(
                    std::forward<TFunction>(function));
                std::future<Result> future = task->get_future();

                // Running nested work inline prevents a worker from blocking while all other
                // workers are themselves waiting for nested jobs from this executor.
                if (TaskExecutor::IsWorkerThread())
                {
                    (*task)();
                    return future;
                }

                {
                    std::lock_guard<std::mutex> lock(this->mutex_);
                    this->tasks_.emplace([task]() { (*task)(); });
                }
                this->condition_.notify_one();
                return future;
            }

            std::size_t GetWorkerCount() const { return this->workers_.size(); }

            TaskExecutor(const TaskExecutor&) = delete;
            TaskExecutor& operator=(const TaskExecutor&) = delete;

        private:
            TaskExecutor();
            ~TaskExecutor();
            static bool IsWorkerThread();
            void WorkerLoop();

            std::vector<std::thread> workers_;
            std::queue<std::function<void()>> tasks_;
            mutable std::mutex mutex_;
            std::condition_variable condition_;
            bool stopping_{false};
        };
    }
}
