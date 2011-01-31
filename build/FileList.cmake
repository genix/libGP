set(Core_HEADER_FILES
    ${PROJECT_SOURCE_DIR}/include/gpdefines.h
    ${PROJECT_SOURCE_DIR}/include/gpenvironment.h
    ${PROJECT_SOURCE_DIR}/include/gpfunctionlookup.h
    ${PROJECT_SOURCE_DIR}/include/gpstats.h
    ${PROJECT_SOURCE_DIR}/include/gptree.h
)

set(Core_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/src/gpenvironment.cpp
    ${PROJECT_SOURCE_DIR}/src/gpfunctionlookup.cpp
    ${PROJECT_SOURCE_DIR}/src/gpglobals.cpp
    ${PROJECT_SOURCE_DIR}/src/gpstats.cpp
    ${PROJECT_SOURCE_DIR}/src/gptree.cpp
)

set(Aux_HEADER_FILES
    ${PROJECT_SOURCE_DIR}/auxiliary/gpreporting.h
)

set(Aux_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/auxiliary/gpreporting.cpp
)

set(Example1_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/examples/example1.cpp
)

set(Example2_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/examples/example2.cpp
)
