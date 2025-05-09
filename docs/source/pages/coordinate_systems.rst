Coordinate Systems
==================

Coordinate Systems
------------------

As far as libCZI is concerned, there are two coordinate systems that are of interest:

1. The *raw-subblock-coordinate-system* is the coordinate system in which the X-Y-positions of the subblocks
   are physically stored in the file, and it is also the coordinate system which is used in libCZI.
   The API `ISubBlock::GetSubBlockInfo <https://zeiss.github.io/libczi/classlib_c_z_i_1_1_i_sub_block.html#a557108549db08e25b1df1ef8fae37a07>`_
   is returning the X-Y-position of the subblock in this coordinate system
   (`SubBlockInfo::logicalRect <https://zeiss.github.io/libczi/structlib_c_z_i_1_1_sub_block_info.html>`_), which is
   unmodified with respect to the actual file content.

   Conceptually, the *raw-subblock-coordinate-system* has the following characteristics:
   * The X and Y-axis are such that the subblock's logicalRect are axis-aligned.
   * Orientation of the Y-axis is such that the Y-coordinate increases from top to bottom; X-coordinates increase from left to right.
   * The origin of the coordinate system is arbitrary, there is no special geometric meaning to the origin.

   .. image:: ../_static/images/raw_subblock_coordinate_system_400x.png
      :alt: Raw SubBlock Coordinate System

2. The *CZI-Pixel-Coordinate-System* is a coordinate system where the top-left subblock (of pyramid-layer 0) has the
   coordinate (0,0). It is related to the *raw-subblock-coordinate-system* by a translation.
   The *CZI-Pixel-Coordinate-System* is the recommended way for relating to an X-Y-position in the CZI-document.

   The characteristics of the *CZI-Pixel-Coordinate-System* are:
   * The X and Y-axis are such that the subblock's logicalRect are axis-aligned.
   * Orientation of the Y-axis is such that the Y-coordinate increases from top to bottom; X-coordinates increase from left to right.
   * The origin is such that the coordinate (of the logicalRect) of the top-most and left-most pyramid-layer0 subblock is (0,0).

   .. image:: ../_static/images/CZI_pixel_coordinate_system_400x.png
      :alt: CZI Pixel Coordinate System

Usage in libCZI
---------------

libCZI at this point is using the *raw-subblock-coordinate-system* for all its operations. There are only a few
operations where the choice of coordinate system is relevant: the accessor-functions which give tile-compositions.
This includes:
* `ISingleChannelTileAccessor <https://zeiss.github.io/libczi/classlib_c_z_i_1_1_i_single_channel_tile_accessor.html>`_
* `ISingleChannelPyramidLayerTileAccessor <https://zeiss.github.io/libczi/classlib_c_z_i_1_1_i_single_channel_pyramid_layer_tile_accessor.html>`_
* `ISingleChannelScalingTileAccessor <https://zeiss.github.io/libczi/classlib_c_z_i_1_1_i_single_channel_scaling_tile_accessor.html>`_

The coordinates of the ROIs passed in here are in the *raw-subblock-coordinate-system*.

Conversion
----------

The primary source of information regarding the "placement of subblocks on the XY-plane" is the method
`ISubBlockRepository::GetStatistics() <https://zeiss.github.io/libczi/classlib_c_z_i_1_1_i_sub_block_repository.html#a6e44c1a929a27036ef77195d516dd719>`_.
The structure `SubBlockStatistics <https://zeiss.github.io/libczi/structlib_c_z_i_1_1_sub_block_statistics.html>`_ contains two
rectangles:

.. list-table::
   :header-rows: 1

   * - Property
     - Description
   * - ``boundingBox``
     - The minimal AABB (= axis-aligned-bounding-box) of the logicalRects of all subblocks (in all pyramid-layers).
   * - ``boundingBoxLayer0Only``
     - The minimal AABB of the logicalRects of all subblocks in pyramid-layer 0.

Converting from the *raw-subblock-coordinate-system* to the *CZI-Pixel-Coordinate-System* is now straightforward:
subtract the top-left point of the ``boundingBoxLayer0Only`` rectangle. The translation vector between the coordinate systems
is then given by the top-left point of the ``boundingBoxLayer0Only`` rectangle.

Translating between both coordinate systems might be necessary when coordinate information is given from an external source
(or by information embedded in the CZI itself). Since the *CZI-Pixel-Coordinate-System* is the recommended way to relate to
spatial positions in a CZI, such external data will most likely be given in this coordinate system.
Within libCZI itself, no coordinate transformation is taking place so far, so all operations are consistently done in the
*raw-subblock-coordinate-system*.