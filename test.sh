mv bin/libpylua.so tests/pylua.so
cd tests

FILE=luaunit.lua

echo "Finding $FILE"

if [ -f "$FILE" ]; then
    echo "$FILE exists."
else 
    echo "$FILE does not exist."
    echo "Downloading $FILE"
    wget https://raw.githubusercontent.com/bluebird75/luaunit/master/luaunit.lua
fi

printf "Running Tests\n"

printf "\nTest Case 1\n"
if ! lua test0.lua; then
    echo "Error"
fi

printf "\nTest Case 2\n"
if ! lua test1.lua; then
    exit -1
fi

printf "\nTest Case 3\n"
if ! lua test2.lua; then
    exit -1
fi

printf "\nTest Case 4\n"
if ! lua test3.lua; then
    exit -1
fi

printf "\n\nFinished Running all tests\n"
