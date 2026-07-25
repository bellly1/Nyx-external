// easyauth C++ client library — public API.
//
// A program using easyauth depends on ONLY this library: all third-party code
// (libsodium, mbedTLS) is statically linked into the built lib. Include this
// single header and link the single static lib:
//
//   #include <easyauth/auth.hpp>
//   easyauth::Client auth("app-id", SERVER_PUBLIC_KEY_B64);
//   auto r = auth.Register("user", "pass", "XXXXX-XXXXX-XXXXX-XXXXX");
//   if (auth.IsAuthenticated()) { ... }
//
// The API host is fixed inside the library (api.easyauth.site). Clients cannot
// point the SDK at another server.
//
// Design guarantee: the library is SERVER-AUTHORITATIVE. It never decides
// whether a key/session is valid. It gathers the HWID, talks to the server, and
// verifies the server's Ed25519 signature on every response. If a signature
// does not verify, the response is rejected. The API is thread-safe and never
// lets exceptions cross the boundary — every call returns a Result.
#ifndef EASYAUTH_AUTH_HPP
#define EASYAUTH_AUTH_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace easyauth {

// Semver of this C++ library. Sent on every auth call as client_version so the
// server can reject outdated builds (admin-configurable minimum).
inline constexpr const char* kClientLibVersion = "1.2.0";

// Remote file metadata from POST /files (Enterprise).
struct FileInfo {
    std::string name;
    long long size = 0;
    std::string sha256;
    std::string content_type;
    long long updated_at = 0;
    std::string data; // base64 payload; set on POST /file only
};

// Result is returned by every network call. `ok` is true only when the request
// succeeded end-to-end AND (where applicable) the license/session is valid.
struct Result {
    bool ok = false;
    std::string error;      // human-readable failure reason ("" on success)
    bool valid = false;     // session/license currently valid
    std::string token;      // opaque session token (also cached locally)
    long long expires_at = 0; // license expiry (unix seconds); 0 = lifetime/none
    int level = 0;          // license level/tier
    std::string username;   // echoed on register/login
    // Merged app + user variables from POST /vars (user overrides app on clash).
    std::map<std::string, std::string> vars;
    std::vector<FileInfo> files; // POST /files
    FileInfo file;               // POST /file (data filled)
};

// Config holds per-app credentials. The API host is not configurable — it is
// compiled into the library.
struct Config {
    std::string app_id;
    std::string server_public_key_b64;  // embedded Ed25519 public key (base64)
    std::string app_secret_b64;         // optional HMAC secret (base64)
    std::string cache_path;             // optional; a per-user default is used if empty

    // TLS options (HTTPS only):
    bool tls_insecure = false;          // skip cert verification (DEV ONLY)
    std::string ca_pem;                 // optional pinned CA bundle (PEM)
    int timeout_ms = 15000;
};

class Client {
public:
    explicit Client(const Config& cfg);
    // Convenience matching the short form (no request HMAC, default cache).
    Client(const std::string& app_id, const std::string& server_public_key_b64);
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    // Network operations. Each verifies the server's signature before trusting
    // the response, and caches the session on success.
    Result Register(const std::string& username, const std::string& password,
                    const std::string& key);
    Result Login(const std::string& username, const std::string& password);
    Result RedeemKey(const std::string& key);
    Result Validate();        // re-checks the current session with the server
    Result GetVars();         // fetch merged app + user variables for this session
    Result ListFiles();       // Enterprise: list remote files for this session
    Result GetFile(const std::string& name); // Enterprise: download one file (base64 in result.file.data)
    bool IsAuthenticated();   // == Validate().ok (consults the server)
    Result Logout();

    // Optional integrity pin: SHA-256 hex of your binary/module. Sent on Validate
    // when set. Required by the server if the app has any hashes configured.
    void SetIntegrityHash(const std::string& sha256_hex);
    void ClearIntegrityHash();

    // The stable HWID for this machine (lowercase hex SHA-256).
    std::string GetHWID();

    // This library's semver (same as kClientLibVersion).
    static std::string LibVersion();

    // Encrypted local session cache. Register/Login/RedeemKey auto-save on
    // success; call LoadSession() at startup to restore a previous login.
    bool LoadSession();       // load+decrypt cache (no network); false if absent/foreign
    bool SaveSession();       // persist the current session
    void ClearSession();      // delete cache + forget the session

    // Cached state (no network).
    bool HasSession() const;
    long long ExpiresAt() const;
    int Level() const;
    std::string Username() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Diagnostics.
bool init();
std::string libsodium_version();
std::string mbedtls_version();
std::string client_lib_version(); // == kClientLibVersion

} // namespace easyauth

#endif // EASYAUTH_AUTH_HPP
