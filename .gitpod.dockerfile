FROM gitpod/workspace-full

# install dependencies
RUN sudo apt-get update \
 && sudo apt-get install -y \
    python3-dev \
    libclang-dev \
    clangd-12 \
    bear \
    manpages-posix-dev \
    manpages-dev \
    python-pygments \
    gcc \
    tar \
    make \
    wget \
    && sudo rm -rf /var/lib/apt/lists/*

# downloading and extracting premake5
RUN sudo wget https://github.com/premake/premake-core/releases/download/v5.0.0-beta1/premake-5.0.0-beta1-linux.tar.gz \
 && sudo tar xzf premake-5.0.0-beta1-linux.tar.gz \
 && sudo mv premake5 /usr/bin \
 && sudo rm premake-5.0.0-beta1-linux.tar.gz

# Compiling lua5.4
RUN wget http://www.lua.org/ftp/lua-5.4.3.tar.gz \
  && tar -xf lua-5.4.3.tar.gz \
  && cd lua-5.4.3 \
  && wget https://gist.githubusercontent.com/Vipul-Cariappa/2e957b77f5d6dde7c0b6d01c04beb911/raw/e65c95968cf36503f6cbc6819761a6b2220f3b69/premake5.lua \
  && premake5 gmake \
  && make \
  && sudo cp bin/lib/Debug/liblua.so /usr/lib/ \
  && sudo cp bin/exec/interpreter/Debug/luai /usr/bin/lua \
  && sudo cp bin/exec/compiler/Debug/luac /usr/bin/luac \
  && sudo cp -r src/ /usr/include/lua5.4/ \
  && cd .. \
  && rm -fr lua-5.4.3

# downloading and extracting luaunit
RUN wget https://raw.githubusercontent.com/bluebird75/luaunit/master/luaunit.lua \
  && mv luaunit.lua tests/
