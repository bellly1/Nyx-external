EasyAuth C++ client (x64 /MD Release) — lib 1.2.0
=================================================

What's in the zip:
  lib/easyauth-bundled.lib         static lib (sodium + mbedTLS inside)
  include/easyauth/auth.hpp        public API
  include/easyauth/credentials.hpp this app's id + public key
  easyauth.props                   drop into Visual Studio
  example/main.cpp                 tiny sample

API host is baked into the .lib (api.easyauth.site). You can't retarget it.

HWID is Windows machine SID only (v3). Reset binds from the dashboard.

Visual Studio (2022 / 2025 / Insiders):
  1. Extract this zip.
  2. Property Manager → Release|x64 → Add Existing Property Sheet
     → easyauth.props
  3. Build Release | x64 with /MD (same as the shipped lib).

ws2_32, bcrypt, crypt32, advapi32 link via #pragma comment in the lib.
