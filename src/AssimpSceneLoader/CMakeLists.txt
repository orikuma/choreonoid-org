if(ENABLE_ASSIMP)
  choreonoid_add_library(CnoidAssimpSceneLoader SHARED AssimpSceneLoader.cpp)
  if(ASSIMP_USE_IMPORTED_LIBRARY)
    target_link_libraries(CnoidAssimpSceneLoader CnoidUtil assimp::assimp)
  else()
    target_link_libraries(CnoidAssimpSceneLoader CnoidUtil ${ASSIMP_LIBRARIES})
  endif()
endif()
