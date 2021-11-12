FILE=premake5

echo "Finding $FILE"

if [ -f "$FILE" ]; then
    echo "$FILE exists."
else 
    echo "$FILE does not exist."
    echo "Downloading $FILE"
    wget https://github.com/premake/premake-core/releases/download/v5.0.0-alpha16/premake-5.0.0-alpha16-linux.tar.gz
    tar -xf premake-5.0.0-alpha16-linux.tar.gz
fi

cp .github/premake5.lua ./premake5.lua
./premake5 gmake
make
