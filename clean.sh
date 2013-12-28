#!/bin/bash

# Delete build output.
rm -rf builds

# Delete gyp related files.
rm -rf gyp-mac-tool

# Delete make related files.
rm -rf Makefile
rm -rf *.Makefile
rm -rf *.target.mk

# Delete Visual Studio related files.
rm -rf _ReSharper.*
rm -rf *.sln
rm -rf *.sdf
rm -rf *.suo
rm -rf *.vcxproj
rm -rf *.vcxproj.filters
rm -rf *.vcxproj.user
rm -rf *.vcxproj
rm -rf *.vcxproj.filters

rm -rf deps/_ReSharper.*
rm -rf deps/*.sln
rm -rf deps/*.sdf
rm -rf deps/*.suo
rm -rf deps/*.vcxproj
rm -rf deps/*.filters
rm -rf deps/*.user

rm -rf deps/openssl/_ReSharper.*
rm -rf deps/openssl/*.sln
rm -rf deps/openssl/*.sdf
rm -rf deps/openssl/*.suo
rm -rf deps/openssl/*.vcxproj
rm -rf deps/openssl/*.filters
rm -rf deps/openssl/*.user

rm -rf deps/uv/_ReSharper.*
rm -rf deps/uv/*.sln
rm -rf deps/uv/*.sdf
rm -rf deps/uv/*.suo
rm -rf deps/uv/*.vcxproj
rm -rf deps/uv/*.filters
rm -rf deps/uv/*.user



rm -rf deps/http-parser/_ReSharper.*
rm -rf deps/http-parser/*.sln
rm -rf deps/http-parser/*.sdf
rm -rf deps/http-parser/*.suo
rm -rf deps/http-parser/*.vcxproj
rm -rf deps/http-parser/*.filters
rm -rf deps/http-parser/*.user

# Delete Xcode related files.
rm -rf *.xcodeproj