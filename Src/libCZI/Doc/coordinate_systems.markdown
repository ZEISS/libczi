Coordinate Systems                 {#coordinatesystems}
==================

## coordinate systems

As far as libCZI is concerned, there are two coordinate systems that are of interest:

1. The _raw-subblock-coordinate-system_ is the coordinate system in which the X-Y-positions of the subblocks
    are physically stored in the file, and it is also the coordinate system which is used in libCZI.
    The API [ISubBlock::GetSubBlockInfo](https://zeiss.github.io/libczi/classlib_c_z_i_1_1_i_sub_block.html#a557108549db08e25b1df1ef8fae37a07) is returning the X-Y-position of the subblock in this 
    coordinate system ([SubBlockInfo::logicalRect](https://zeiss.github.io/libczi/structlib_c_z_i_1_1_sub_block_info.html)), which is
    unmodified wrt to the actual file content.   
    Conceptually, the _raw-subblock-coordinate-system_ has the following characteristics:   
    * The X and Y-axis are such that the subblock's logicalRect are axis-aligned.
    * Orientation of the Y-axis is such that the Y-coordinate increases from top to bottom; X-coordinates increase from left to right.
    * The origin of the coordinate system is arbitrary, there is no special geometric meaning to the origin.   
   ![Raw SubBlock Coordinate System](raw_subblock_coordinate_system_400x.png)
2. The _CZI-Pixel-Coordinate-System_ is a coordinate system where the top-left subblock (of pyramid-layer 0) has the
   the coordinate (0,0). It is related to the _raw_subblock-coordinate system_ by a translation.   
   The _CZI-Pixel-Coordinate-System_ is the recommended way for relating to an X-Y-position in the CZI-document.   
   The characteristics of the _CZI-Pixel-Coordinate-System_ are:
   * The X and Y-axis are such that the subblock's logicalRect are axis-aligned.
   * Orientation of the Y-axis is such that the Y-coordinate increases from top to bottom; X-coordinates increase from left to right.
   * The origin is such that the coordinate (of the logicalRect) of the top-most and left-most pyramid-layer0 subblock is (0,0).   
   ![CZI Pixel Coordinate System](CZI_pixel_coordinate_system_400x.png)

      
## usage in libCZI

libCZI at this point is using the _raw-subblock-coordinate-system_ for all its operations. There are only a few
operations where the choice of coordinate system is relevant: the accessor-functions which give tile-compositions.   
This includes:
- [ISingleChannelTileAccessor](https://zeiss.github.io/libczi/classlib_c_z_i_1_1_i_single_channel_tile_accessor.html)
- [ISingleChannelPyramidLayerTileAccessor](https://zeiss.github.io/libczi/classlib_c_z_i_1_1_i_single_channel_pyramid_layer_tile_accessor.html)
- [ISingleChannelScalingTileAccessor](https://zeiss.github.io/libczi/classlib_c_z_i_1_1_i_single_channel_scaling_tile_accessor.html)

The coordinates of the ROIs passed in here are in the _raw-subblock-coordinate-system_.

## conversion

The primary source of information regarding the "placement of subblocks on the XY-plane" is the method [ISubBlockRepository::GetStatistics()](https://zeiss.github.io/libczi/classlib_c_z_i_1_1_i_sub_block_repository.html#a6e44c1a929a27036ef77195d516dd719).  
The structure [SubBlockStatistics](https://zeiss.github.io/libczi/structlib_c_z_i_1_1_sub_block_statistics.html) contains two
rectangles:

| property             | description                                                                 |
|----------------------|-----------------------------------------------------------------------------|
| boundingBox          | The minimal AABB (= axis-aligned-bounding-box) of the logicalRects of all subblocks (in all pyramid-layers). |
| boundingBoxLayer0Only| The minimal AABB of the logicalRects of all subblocks in pyramid-layer 0.    |

Converting from the _raw-subblock-coordinate-system_ to the _CZI-Pixel-Coordinate-System_ is now straightforward:
subtract the top-left point of the boundingBoxLayer0Only rectangle. The translation vector between the coordinate systems
is then given by the top-left point of the boundingBoxLayer0Only rectangle.

Translating between both coordinate systems might be necessary when coordinate information is given from an external source
(or by information embedded in the CZI itself). Since the _CZI-Pixel-Coordinate-System_ is the recommended way to relate to
spatial positions in a CZI, such external data will most likely be given in this coordinate system.  
Within libCZI itself, no coordinate transformation is taking place so far, so all operations are consistently done in the
_raw-subblock-coordinate-system_.