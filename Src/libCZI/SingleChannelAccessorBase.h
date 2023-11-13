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

    explicit CSingleChannelAccessorBase(std::shared_ptr<libCZI::ISubBlockRepository> sbBlkRepository)
        : sbBlkRepository(sbBlkRepository)
    {}

    bool TryGetPixelType(const libCZI::IDimCoordinate* planeCoordinate, libCZI::PixelType& pixeltype);

    static void Clear(libCZI::IBitmapData* bm, const libCZI::RgbFloatColor& floatColor);

    void CheckPlaneCoordinates(const libCZI::IDimCoordinate* planeCoordinate) const;

    /// This method is used to do a visibility test of a list of subblocks. The mode of operation is as follows:
    /// - The method is given a ROI, and the number of subblocks to check.  
    /// - The functor 'get_subblock_index' is called with the argument being a counter, starting with 0 and counting up to count-1.  
    ///   If called with value 0, the subblocks is the **last** one to be rendered, if called with value 1, the subblock is the
    ///   second-last one to be rendered, and so on.
    ///   The value it returns is the subblock index (in the subblock repository) to check.
    /// - The subblocks are assumed to be rendered in the order given, so the one we get by calling  'get_subblock_index' with  
    ///   argument 0 is the first one to be rendered, the one with argument 1 is the second one, and so on. The rendering 
    ///   is assumed to be done with the 'painter's algorithm", so what is rendered last is on top.
    /// - We return a list of indices which are to be rendered, potentially leaving out some which have been determined  
    ///   as not being visible. The indices returned are "indices as used by the 'get_subblock_index' functor, i.e.
    ///   it is **not** the subblock-number, but the argument that was passed to the functor.
    /// - The caller can then use this list to render the subblocks (in the order as given in this vector).
    ///
    /// \param  roi     The roi.
    /// \param  count   Number of subblocks (specifying how many times the get_subblock_index-functor is being called.
    /// \param  get_subblock_index Functor which gives the subblock index to check.
    ///
    /// \returns    A list of indices of "arguments to the functor which delivered a visible subblock".
    std::vector<int> CheckForVisibility(const libCZI::IntRect& roi, int count, const std::function<int(int)>& get_subblock_index) const;
};
