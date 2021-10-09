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
lua test0.lua

printf "\nTest Case 2\n"
lua test1.lua

printf "\nTest Case 3\n"
lua test2.lua

printf "\nTest Case 4\n"
lua test3.lua
