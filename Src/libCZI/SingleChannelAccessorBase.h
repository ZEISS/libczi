// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <memory>
#include "libCZI.h"

class CSingleChannelAccessorBase
{
protected:
    std::shared_ptr<libCZI::ISubBlockRepository> sbBlkRepository;

    explicit CSingleChannelAccessorBase(const std::shared_ptr<libCZI::ISubBlockRepository>& sbBlkRepository)
        : sbBlkRepository(sbBlkRepository)
    {}

    bool TryGetPixelType(const libCZI::IDimCoordinate* planeCoordinate, libCZI::PixelType& pixeltype);

    static void Clear(libCZI::IBitmapData* bm, const libCZI::RgbFloatColor& floatColor);

    void CheckPlaneCoordinates(const libCZI::IDimCoordinate* planeCoordinate) const;

    /// This method is used to do a visibility test of a list of subblocks. The mode of operation is as follows:
    /// - The method is given a ROI, and the number of subblocks to check.  
    /// - The functor 'get_subblock_index' is called with the argument being a counter, starting with count-1 and counting down to zero.  
    ///   If called with value  count-1, the subblock is the **last** one to be rendered; if called with value count-2, the subblock is the
    ///   second-to-the-last one to be rendered, and so on. If called with 0, the subblock is the **first** one to be rendered.
    ///   The value it returns is the subblock index (in the subblock repository) to check.
    /// - The subblocks are assumed to be rendered in the order given, so the one we get by calling 'get_subblock_index' with  
    ///   argument 0 is the first one to be rendered, the one with argument 1 is the second one, and so on. The rendering 
    ///   is assumed to be done with the 'painter's algorithm", so what is rendered last is on top.
    /// - We return a list of indices which are to be rendered, potentially leaving out some which have been determined  
    ///   as not being visible. The indices returned are "indices as used by the 'get_subblock_index' functor", i.e.
    ///   it is **not** the subblock-index, but the argument that was passed to the functor.
    /// - The caller can then use this list to render the subblocks (in the order as given in this vector).
    ///
    /// \param  roi     The roi - if this is empty or invalid, then an empty vector is returned.
    /// \param  count   Number of subblocks (specifying how many times the get_subblock_index-functor is being called).
    /// \param  get_subblock_index Functor which gives the subblock index to check. This index is the index in the subblock repository.
    ///
    /// \returns    A list of indices of "arguments to the functor which delivered a visible subblock". If the subblocks are rendered in the order
    ///             given here, then the result is guaranteed to be the same as if all subblocks were rendered.
    std::vector<int> CheckForVisibility(const libCZI::IntRect& roi, int count, const std::function<int(int)>& get_subblock_index) const;

    /// Do a visibility check for a list of subblocks. This is the core method, which is used by the public method 'CheckForVisibility'.
    /// What this function does, is:
    /// - The method is given a ROI, and the number of subblocks to check.    
    /// - The function 'get_subblock_index' will be called with the argument being a counter, starting with count-1 and counting down to zero.  
    ///   If called with value  count-1, the subblock is the **last** one to be rendered; if called with value count-2, the subblock is the
    ///   second-to-the-last one to be rendered, and so on. If called with 0, the subblock is the **first** one to be rendered.
    /// - The value it returned by the functor 'get_subblock_index' is then used with the functor 'get_rect_of_subblock' to get the rectangle. The  
    ///   index returned by 'get_subblock_index' is passed in to the functor 'get_rect_of_subblock'. The rectangle returned by 'get_rect_of_subblock'
    ///   is the rectangle of the subblock, the region where this subblock is rendered.
    /// - We return a list of indices which are to be rendered, potentially leaving out some which have been determined  
    ///   as not being visible. The indices returned are "indices as used by the 'get_subblock_index' functor", i.e.
    ///   it is **not** the subblock-index, but the argument that was passed to the functor.
    ///
    /// \param  roi                     The roi - if this is empty or invalid, then an empty vector is returned.
    /// \param  count                   Number of subblocks (specifying how many times the get_subblock_index-functor is being called).
    /// \param  get_subblock_index      Functor which gives the subblock index for a given counter. The counter starts with count-1 and counts down to zero.
    /// \param  get_rect_of_subblock    Functor which gives the subblock rectangle for a subblock-index (as returned by 'get_subblock_index').
    ///
    /// \returns    A list of indices of "arguments to the functor which delivered a visible subblock". If the subblocks are rendered in the order
    ///             given here, then the result is guaranteed to be the same as if all subblocks were rendered. Non-visible subblocks are not
    ///             part of this list.
    static std::vector<int> CheckForVisibilityCore(const libCZI::IntRect& roi, int count, const std::function<int(int)>& get_subblock_index, const std::function<libCZI::IntRect(int)>& get_rect_of_subblock);

    struct SubBlockData
    {
        std::shared_ptr<libCZI::IBitmapData> bitmap;
        libCZI::SubBlockInfo subBlockInfo;
    };

    static SubBlockData GetSubBlockDataForSubBlockIndex(
        const std::shared_ptr<libCZI::ISubBlockRepository>& sbBlkRepository, 
        const std::shared_ptr<libCZI::ISubBlockCacheOperation>& cache,
        int subBlockIndex,
        bool onlyAddCompressedSubBlockToCache);
};
