# SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

# downlad and build googletest -> https://github.com/google/googletest/tree/main/googletest , https://crascit.com/2015/07/25/cmake-gtest/

# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)

# Prevent overriding the parent project's compiler/linker
# settings on Windows (cf. https://github.com/google/googletest/blob/master/googletest/README.md)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . RESULT_VARIABLE result  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/googletest-download" )

if(result)
	  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} --build . RESULT_VARIABLE result  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )

if(result)
	  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()


# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
					 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
					 EXCLUDE_FROM_ALL)

ADD_EXECUTABLE(libCZI_UnitTests 
										inc_libCZI.h              
										pch.h                          
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
										test_JxrDecode.cpp              
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
										pch.cpp                   
										testImage.cpp                  
										test_reader.cpp                 
										utils.h)

# Now simply link against gtest or gtest_main as needed. Eg
TARGET_LINK_LIBRARIES(libCZI_UnitTests
		libCZIStatic
		gtest
		gmock
)

set_target_properties(libCZI_UnitTests PROPERTIES INCLUDE_DIRECTORIES "${gtest_SOURCE_DIR}/include")

set_target_properties(libCZI_UnitTests PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES) 

add_test(NAME libCZI_UnitTests COMMAND libCZI_UnitTests)

