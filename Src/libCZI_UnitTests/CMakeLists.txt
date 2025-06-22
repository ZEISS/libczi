# SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

ADD_EXECUTABLE(libCZI_UnitTests 
										inc_libCZI.h
										include_gtest.h
										testImage.h                     
										test_readerwriter.cpp
										main.cpp                 
										SegmentWalker.cpp              
										testImageHiLo.cpp               
										test_splines.cpp
										MemInputOutputStream.cpp  
										SegmentWalker.h                
										test_IndexSet.cpp               
										test_StreamImplementations.cpp
										MemInputOutputStream.h    
										test_bitmapOperations.cpp      
										test_JxrlibCodec.cpp              
										test_Utilities.cpp
										MemOutputStream.cpp       
										test_CziSubBlockDirectory.cpp  
										test_LUT.cpp                    
										test_writer.cpp
										MemOutputStream.h         
										test_DimCoordinate.cpp         
										test_metadatabuilder.cpp        
										test_ZstdCompress.cpp
										MockMetadataSegment.cpp   
										test_DisplaySettings.cpp       
										test_metadatareading.cpp        
										test_ZstdDecode.cpp
										MockMetadataSegment.h     
										testImage1.cpp                 
										test_multichannelcomposite.cpp  
										utils.cpp
										testImage.cpp
										test_reader.cpp                 
										utils.h 
										test_Accessors.cpp 
										test_CZIParse.cpp 
										test_streamslib.cpp 
										test_curlhttpstream.cpp
										test_rectanglecoverage.cpp 
										test_TileAccessorCoverageOptimization.cpp 
										test_SubBlockCache.cpp 
										test_azureblobstream.cpp
										test_frame_of_reference_transform.cpp
										test_subblockmetadata.cpp)

TARGET_LINK_LIBRARIES(libCZI_UnitTests PRIVATE libCZIStatic GTest::gtest GTest::gmock)
set_target_properties(libCZI_UnitTests PROPERTIES CXX_STANDARD 14)

target_compile_definitions(libCZI_UnitTests PRIVATE _LIBCZISTATICLIB)

add_test(NAME libCZI_UnitTests COMMAND libCZI_UnitTests)
