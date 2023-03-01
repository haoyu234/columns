project(
    'clcli', 
    'c',
    version: 'v0.0.1',
    default_options: 'cpp_std=c++11'
)

columns_dep = declare_dependency(
    include_directories: 'include',
    link_with: static_library(
        'columns',
        sources: 'src/columns.c',
        include_directories: 'include'
    ),
)
