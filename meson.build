project(
    'columns', 
    ['c', 'cpp'],
    version: 'v0.0.1',
    default_options: 'c_std=c11'
)

columns_dep = declare_dependency(
    include_directories: 'include',
    link_with: static_library(
        'columns',
        sources: 'src/columns.c',
        include_directories: 'include'
    ),
)

test(
    'kind',
    executable(
        'kindcpp',
        sources: 'tests/kind.cpp',
        override_options: '-cpp_std=c++11',
        dependencies: [
            columns_dep,
            dependency('gtest', main: true)
        ]
    )
)

test(
    'kind2',
    executable(
        'kindc',
        sources: 'tests/kind.c',
        override_options: '-c_std=c11',
        dependencies: [
            columns_dep,
            dependency('cunit')
        ]
    )
)