project 'kernel'
    kind 'ConsoleApp'
	defines { 'PACKETNGIN_SINGLE' }

    build.compileProperty('x86_64')
    removefiles { 'src/**.S' }

    build.linkingProperty { 'hal', 'ext', 'tlsf', 'lwip', 'vnic' }

    targetname  'kernel.elf'
    linkoptions { '-T elf_x86_64.ld -e main' }

    -- Make version header
    prebuildcommands {'../scripts/mkver.sh > src/version.h',
			'gcc -D__ASSEMBLY__ -o obj/entry.o -c src/entry.S' }

    filter 'configurations:debug'
        removefiles { 'src/test/**.c', 'src/test/**.asm' }

    filter 'configurations:release'
        removefiles { 'src/test/**.c', 'src/test/**.asm' }

    filter {}

--[[
   [        filter 'configurations:Debug'
   [            prebuildcommands {
   [                'java -jar ../../tools/AceUnit-0.12.0.jar ../src/test'
   [            }
   [
   [            files {
   [                'src/test.c', 'src/test/**.c', 'src/test/**.h',
   [                '../tools/aceunit/src/AceUnit.c', 
   [                '../tools/aceunit/src/AceUnitData.c', 
   [                '../tools/aceunit/src/loggers/FullPlainLogger.c', 
   [                '../tools/aceunit/include/**.h'}
   [
   [            defines { 'TEST', 'ACEUNIT_ASSERTION_STYLE=ACEUNIT_ASSERTION_STYLE_RETURN', 'ACEUNIT_SUITES' }
   [            includedirs { '../tools/aceunit/include' }
   ]]
