# Find the OpenGL Mathematics library (GLM)
# This is only really neede for Windows I think, since GLM installs
# to the regular lib/include dirs on Linux so no extra work is needed
# on Windows the GLM root dir should be set as an environment variable
# named GLM

find_path(GLM_INCLUDE_DIR glm/glm.hpp
	HINTS $ENV{GLM}
	PATH_SUFFIXES glm
)
find_library(GLM_LIBRARY
	HINTS $ENV{GLM}
	NAMES glm
	PATH_SUFFIXES lib bin
)

set(GLM_INCLUDE_DIRS ${GLM_INCLUDE_DIR})
set(GLM_LIBRARIES ${GLM_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLM REQUIRED_VARS GLM_INCLUDE_DIR GLM_LIBRARY)

mark_as_advanced(GLM_INCLUDE_DIR GLM_LIBRARY)

