project(
    'columns', 
    ['c', 'cpp'],
    version: 'v0.0.1',
    default_options: 'c_std=c11'
)

columns_dep = declare_dependency(
    include_directories: 'include',
)

if get_option('enable-tests')
    test(
        'test1',
        executable(
            'test1',
            sources: 'tests/test1.cpp',
            override_options: '-cpp_std=c++11',
            dependencies: [
                columns_dep,
                dependency('gtest', main: true)
            ]
        )
    )

    test(
        'test2',
        executable(
            'test2',
            sources: 'tests/test2.c',
            override_options: '-c_std=c11',
            dependencies: [
                columns_dep,
                dependency('cunit')
            ]
        )
    )

    test(
        'test3',
        executable(
            'test3',
            sources: [
                'tests/test3.cpp',
                'tests/messages_def.c',
            ],
            override_options: '-cpp_std=c++11',
            dependencies: [
                columns_dep,
                dependency('gtest', main: true)
            ]
        )
    )
endif