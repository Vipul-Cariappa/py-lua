echo "Deleting pylua.dll"
rm -f ./pylua.dll

echo "Copying new pylua.dll"
cp "C:\Users\Vipul Cariappa\Documents\pylua\x64\Release\pylua.dll" ./pylua.dll

echo "Running lua tests"
lua ./test.lua
