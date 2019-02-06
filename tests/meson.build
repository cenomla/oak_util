
memory_test = executable(
	'memory', 'memory.cpp',
	objects: oak_util.extract_all_objects(),
	include_directories: includes,
	dependencies: deps)

hash_test = executable(
	'hash', 'hash_container.cpp',
	objects: oak_util.extract_all_objects(),
	include_directories: includes,
	dependencies: deps)

test('memory', memory_test)
test('hash_container', hash_test)

