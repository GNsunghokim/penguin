project 'collection'
    kind 'StaticLib'

    build.compileProperty('x86_64')
    build.linkingProperty()
    build.targetPath('..')

    buildoptions    '-fms-extensions'

project 'collectiontest'
    kind 'ConsoleApp'
    language 'C'
	location '.'

    includedirs "include"
	files { "test/**.h", "test/**.c" }
    links { 'collection' }

	postbuildcommands { "%{cfg.buildtarget.abspath}" } -- run tester
    buildoptions    '-fms-extensions'

