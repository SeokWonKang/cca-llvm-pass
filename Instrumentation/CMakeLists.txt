#-----------------------
# Targets
#-----------------------
# Library
add_library( PIMCCALLVMInstrumentation MODULE 
	Utils.cpp
	# Fixed/CCAMulAdd.cpp
	# Fixed/CCAMulAddDouble.cpp
	# Fixed/CCAMulSubMulDiv.cpp
	CCAPatternGraph.cpp
	parser/cca.tab.cc
	parser/cca.lex.cc
	CCAUniversal.cpp
	PassPlugin.cpp )
target_include_directories( PIMCCALLVMInstrumentation PUBLIC
	$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include> )

add_executable( parserTester 
	parser/tester.cpp
	CCAPatternGraph.cpp
	parser/cca.tab.cc
	parser/cca.lex.cc
	Utils.cpp )
target_include_directories( parserTester PUBLIC
	$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include> )

#------------------------------
# Install
#------------------------------
install( 
	FILES       RemoveComment.sh
	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install( 
	TARGETS  PIMCCALLVMInstrumentation
	EXPORT   ${EXPORT_NAME} 
	INCLUDES DESTINATION ${CMAKE_INSTALL_PREFIX}/include
	RUNTIME  DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
	LIBRARY  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
	ARCHIVE  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib )
