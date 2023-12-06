# SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

include(FetchContent)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(${CMAKE_VERSION}  VERSION_GREATER_EQUAL "3.24.0")
	FetchContent_Declare(
		googletest
		URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
		DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
else ()
	FetchContent_Declare(
		googletest
		URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
endif()

FetchContent_MakeAvailable(googletest)

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
										test_SubBlockCache.cpp)

TARGET_LINK_LIBRARIES(libCZI_UnitTests PRIVATE libCZIStatic GTest::gtest GTest::gmock)

target_compile_definitions(libCZI_UnitTests PRIVATE _LIBCZISTATICLIB)

add_test(NAME libCZI_UnitTests COMMAND libCZI_UnitTests)
