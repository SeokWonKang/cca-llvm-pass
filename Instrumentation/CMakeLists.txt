#-----------------------
# Targets
#-----------------------
# Library
add_library( PIMCCALLVMInstrumentation MODULE 
	InsertCommenttedInlineBeforeRet.cpp )

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
