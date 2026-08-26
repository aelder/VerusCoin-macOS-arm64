
#include "params.h"
#include "ui_interface.h"
#include "pbaas/vdxf.h"
#include "univalue.h"
#include "utilstrencodings.h"
#include <cctype>
#include <fstream>
#include <iterator>

std::map<std::string, ParamFile> mapParams;
JsonDownload downloadedJSON;
static const int K_READ_BUF_SIZE{ 1024 * 16 };

std::string CalcSha256(std::string filename)
{
    // Initialize openssl
    SHA256_CTX context;
    if(!SHA256_Init(&context)) {
        return "";
    }

    // Read file and update calculated SHA
    char buf[K_READ_BUF_SIZE];
    std::ifstream file(filename, std::ifstream::binary);
    while (file.good()) {
        file.read(buf, sizeof(buf));
        if(!SHA256_Update(&context, buf, file.gcount())) {
            return "";
        }
    }

    // Get Final SHA
    unsigned char result[SHA256_DIGEST_LENGTH];
    if(!SHA256_Final(result, &context)) {
        return "";
    }

    // Transform byte-array to string
    std::stringstream shastr;
    shastr << std::hex << std::setfill('0');
    for (const auto &byte: result) {
        shastr << std::setw(2) << (int)byte;
    }
    return shastr.str();
}


bool checkParams() {
    bool allVerified = true;
    for (std::map<std::string, ParamFile>::iterator it = mapParams.begin(); it != mapParams.end(); ++it) {
        std::string uiMessage = "Verifying " + it->second.name + "....";
        uiInterface.InitMessage(_(uiMessage.c_str()));

        std::string sha256Sum = CalcSha256(it->second.path.string());

        LogPrintf("sha256Sum %s\n", sha256Sum);
        LogPrintf("checkSum %s\n", it->second.hash);

        if (sha256Sum == it->second.hash) {
            it->second.verified = true;
        } else {
            allVerified = false;
        }
    }
    return allVerified;
}


static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}



static int xferinfo(void *p,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t ultotal, curl_off_t ulnow)
{
    struct CurlProgress *myp = (struct CurlProgress *)p;
    CURL *curl = myp->curl;
    TIMETYPE curtime = 0;

    char *url = NULL;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);

    std::map<std::string, ParamFile>::iterator mi = mapParams.find(url);
    if (mi != mapParams.end()) {
        mi->second.dlnow = dlnow;
        mi->second.dltotal = dltotal;
    }

    return 0;
}

uint160 vARRRChainID()
{
    static uint160 vARRRID = CVDXF::GetID("vARRR.vrsc.@");
    return vARRRID;
}

uint160 vDEXChainID()
{
    static uint160 vARRRID = CVDXF::GetID("vDEX.vrsc.@");
    return vARRRID;
}

uint160 ChipsChainID()
{
    static uint160 ChipsID = CVDXF::GetID("chips.vrsc.@");
    return ChipsID;
}

void initalizeMapParamBootstrap() {
    mapParams.clear();

    ParamFile bootSigFile;
    bootSigFile.name = "bootstrap-signature";
    bootSigFile.verified = false;
    if (_IsVerusMainnetActive())
    {
        bootSigFile.URL = "https://bootstrap.verus.io/VRSC-bootstrap.tar.gz.verusid";
        bootSigFile.path = GetDataDir() / "VRSC-bootstrap.tar.gz.verusid";
    }
    else if (_IsVerusActive())
    {
        bootSigFile.URL = "https://bootstrap.verustest.net/vrsctest-bootstrap.tar.gz.verusid";
        bootSigFile.path = GetDataDir() / "verustest-bootstrap.tar.gz.verusid";
    }
    else if (_IsCurrentChainID(vARRRChainID()))
    {
        bootSigFile.URL = "https://bootstrap.dexstats.info/VARRR-bootstrap.tar.gz.verusid";
        bootSigFile.path = GetDataDir() / "VARRR-bootstrap.tar.gz.verusid";
    }
    else if (_IsCurrentChainID(vDEXChainID()))
    {
        bootSigFile.URL = "https://bootstrap.dexstats.info/VDEX-bootstrap.tar.gz.verusid";
        bootSigFile.path = GetDataDir() / "VDEX-bootstrap.tar.gz.verusid";
    }
    else if (_IsCurrentChainID(ChipsChainID()))
    {
        bootSigFile.URL = "https://bootstrap.dexstats.info/CHIPS-bootstrap.tar.gz.verusid";
        bootSigFile.path = GetDataDir() / "CHIPS-bootstrap.tar.gz.verusid";
    }

    bootSigFile.dlnow = 0;
    bootSigFile.dltotal = 0;
    mapParams[bootSigFile.URL] = bootSigFile;

    ParamFile bootFile;
    bootFile.name = "bootstrap";
    bootFile.verified = false;
    if (_IsVerusMainnetActive())
    {
        bootFile.URL = "https://bootstrap.verus.io/VRSC-bootstrap.tar.gz";
        bootFile.path = GetDataDir() / "VRSC-bootstrap.tar.gz";
    }
    else if (_IsVerusActive())
    {
        bootFile.URL = "https://bootstrap.verustest.net/vrsctest-bootstrap.tar.gz";
        bootFile.path = GetDataDir() / "verustest-bootstrap.tar.gz";
    }
    else if (_IsCurrentChainID(vARRRChainID()))
    {
        bootFile.URL = "https://bootstrap.dexstats.info/VARRR-bootstrap.tar.gz";
        bootFile.path = GetDataDir() / "VARRR-bootstrap.tar.gz";
    }
    else if (_IsCurrentChainID(vDEXChainID()))
    {
        bootFile.URL = "https://bootstrap.dexstats.info/VDEX-bootstrap.tar.gz";
        bootFile.path = GetDataDir() / "VDEX-bootstrap.tar.gz";
    }
    else if (_IsCurrentChainID(ChipsChainID()))
    {
        bootFile.URL = "https://bootstrap.dexstats.info/CHIPS-bootstrap.tar.gz";
        bootFile.path = GetDataDir() / "CHIPS-bootstrap.tar.gz";
    }

    bootFile.dlnow = 0;
    bootFile.dltotal = 0;
    mapParams[bootFile.URL] = bootFile;
}


void initalizeMapParam() {

    mapParams.clear();

    ParamFile spendFile;
    spendFile.name = "sapling-spend.params";
    spendFile.URL = SAPLING_SPEND_URL;
    spendFile.hash = SAPLING_SPEND_SHA256;
    spendFile.verified = false;
    spendFile.path = ZC_GetParamsDir() / "sapling-spend.params";
    spendFile.dlnow = 0;
    spendFile.dltotal = 0;
    mapParams[spendFile.URL] = spendFile;

    ParamFile outputFile;
    outputFile.name = "sapling-output.params";
    outputFile.URL = SAPLING_OUTPUT_URL;
    outputFile.hash = SAPLING_OUTPUT_SHA256;
    outputFile.verified = false;
    outputFile.path = ZC_GetParamsDir() / "sapling-output.params";
    outputFile.dlnow = 0;
    outputFile.dltotal = 0;
    mapParams[outputFile.URL] = outputFile;

    ParamFile groth16File;
    groth16File.name = "sprout-groth16.params";
    groth16File.URL = SPROUT_GROTH16_URL;
    groth16File.hash = SPROUT_GROTH16_SHA256;
    groth16File.verified = false;
    groth16File.path = ZC_GetParamsDir() / "sprout-groth16.params";
    groth16File.dlnow = 0;
    groth16File.dltotal = 0;
    mapParams[groth16File.URL] = groth16File;

}

bool downloadFiles(std::string title)
{
    if (!exists(ZC_GetParamsDir())) {
        create_directory(ZC_GetParamsDir());
    }

    for (std::map<std::string, ParamFile>::iterator it = mapParams.begin(); it != mapParams.end(); ++it) {
        if (!it->second.verified) {
            //open file for writing
            it->second.file = fopen(it->second.path.string().c_str(), "wb");
            if (!it->second.file) {
                return false;
            }
        }
    }

    bool downloadComplete;
    curl_global_init(CURL_GLOBAL_ALL);

    for (int i = 0; i < 500; i++) {

        downloadComplete = true;

        CURLM *multi_handle;
        multi_handle = curl_multi_init();
        int still_running = 0; /* keep number of running handles */

        for (std::map<std::string, ParamFile>::iterator it = mapParams.begin(); it != mapParams.end(); ++it) {

            if (!it->second.verified) {
                /* init the curl session */
                it->second.curl = curl_easy_init();
                if(it->second.curl) {
                    it->second.prog.lastruntime = 0;
                    it->second.prog.curl = it->second.curl;
                }

                curl_easy_setopt(it->second.curl, CURLOPT_URL, it->second.URL.c_str());
                curl_easy_setopt(it->second.curl, CURLOPT_SSL_VERIFYPEER, 1L);
                curl_easy_setopt(it->second.curl, CURLOPT_SSL_VERIFYHOST, 2L);
                curl_easy_setopt(it->second.curl, CURLOPT_VERBOSE, 0L);
                curl_easy_setopt(it->second.curl, CURLOPT_TCP_KEEPALIVE, 1L);
                curl_easy_setopt(it->second.curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
                curl_easy_setopt(it->second.curl, CURLOPT_XFERINFODATA, &it->second.prog);
                curl_easy_setopt(it->second.curl, CURLOPT_NOPROGRESS, 0L);
                curl_easy_setopt(it->second.curl, CURLOPT_WRITEFUNCTION, write_data);
                curl_easy_setopt(it->second.curl, CURLOPT_WRITEDATA, it->second.file);
                curl_easy_setopt(it->second.curl, CURLOPT_RESUME_FROM_LARGE, it->second.dlretrytotal);
                curl_multi_add_handle(multi_handle, it->second.curl);
            }
        }

        curl_multi_perform(multi_handle, &still_running);

        std::string uiMessage;
        uiMessage = "Downloading " + title + "......0.00%";
        uiInterface.InitMessage(_(uiMessage.c_str()));
        int64_t nNow = GetTime();

        while(still_running) {

          if (ShutdownRequested()) {
              downloadComplete = false;
              break;
          }

          if (GetTime() >= nNow + 2) {
              nNow = GetTime();
              int64_t dltotal = 0;
              int64_t dlnow = 0;
              for (std::map<std::string, ParamFile>::iterator it = mapParams.begin(); it != mapParams.end(); ++it) {
                  if (!it->second.verified) {
                      dltotal += it->second.dltotal + it->second.dlretrytotal;
                      dlnow += it->second.dlnow + it->second.dlretrytotal;
                  }
              }
              double pert = 0.00;
              if (dltotal > 0) {
                  pert = (dlnow / (double)dltotal) * 100;
              }
              uiMessage = "Downloading " + title + "......" + std::to_string(pert).substr(0,10) + "%";
              uiInterface.InitMessage(_(uiMessage.c_str()));
              if (title == "Bootstrap" && dltotal > 0) {
                  LogPrintf("bootstrap transfer: %lld/%lld\n",
                            (long long)dlnow,
                            (long long)dltotal);
              }
          }

          struct timeval timeout;
          int rc; /* select() return code */
          CURLMcode mc; /* curl_multi_fdset() return code */

          fd_set fdread;
          fd_set fdwrite;
          fd_set fdexcep;
          int maxfd = -1;

          long curl_timeo = 5;

          FD_ZERO(&fdread);
          FD_ZERO(&fdwrite);
          FD_ZERO(&fdexcep);

          /* set a suitable timeout to play around with */
          timeout.tv_sec = 1;
          timeout.tv_usec = 0;

          curl_multi_timeout(multi_handle, &curl_timeo);
          if(curl_timeo >= 0) {
            timeout.tv_sec = curl_timeo / 1000;
            if(timeout.tv_sec > 1)
              timeout.tv_sec = 1;
            else
              timeout.tv_usec = (curl_timeo % 1000) * 1000;
          }

          /* get file descriptors from the transfers */
          mc = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

          if(mc != CURLM_OK) {
            fprintf(stderr, "curl_multi_fdset() failed, code %d.\n", mc);
            downloadComplete = false;
            break;
          }

          /* On success the value of maxfd is guaranteed to be >= -1. We call
             select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
             no fds ready yet so we call select(0, ...) --or Sleep() on Windows--
             to sleep 100ms, which is the minimum suggested value in the
             curl_multi_fdset() doc. */

          if(maxfd == -1) {
    #ifdef _WIN32
            Sleep(100);
            rc = 0;
    #else
            /* Portable sleep for platforms other than Windows. */
            struct timeval wait = { 0, 100 * 1000 }; /* 100ms */
            rc = select(0, NULL, NULL, NULL, &wait);
    #endif
          }
          else {
            /* Note that on some platforms 'timeout' may be modified by select().
               If you need access to the original value save a copy beforehand. */
            rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
          }

          switch(rc) {
          case -1:
            downloadComplete = false;
            break;
          case 0:
          default:
            /* timeout or readable/writable sockets */
            curl_multi_perform(multi_handle, &still_running);
            break;
          }
        }

        if (downloadComplete)
        for (std::map<std::string, ParamFile>::iterator it = mapParams.begin(); it != mapParams.end(); ++it) {
            if (!it->second.verified) {
                it->second.dlretrytotal += it->second.dlnow;
                curl_easy_cleanup(it->second.curl);
                if (it->second.dlnow != it->second.dltotal) {
                    downloadComplete = false;
                }
            }
        }

        curl_multi_cleanup(multi_handle);
        curl_global_cleanup();

        if (downloadComplete)
            break;

        if (ShutdownRequested()) {
            downloadComplete = false;
            break;
        }
        LogPrintf("Retrying Download - Retry #%d\n", i);
    }

    for (std::map<std::string, ParamFile>::iterator it = mapParams.begin(); it != mapParams.end(); ++it) {
        if (!it->second.verified) {
            fclose(it->second.file);

        }
    }

    return downloadComplete;
}

static size_t writer(char *in, size_t size, size_t nmemb, std::string *out)
{
      out->append((char*)in, size * nmemb);
      return size * nmemb;
}

void getHttpsJson(std::string url)
{
    {
        JsonDownload newDownload;
        downloadedJSON = newDownload;
    }

    downloadedJSON.failed = false;
    downloadedJSON.complete = false;
    downloadedJSON.URL = url;
    std::string response_string;

    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl;
    CURLcode res;

    struct curl_slist *headers=NULL; // init to NULL is important

    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "charset: utf-8");

    curl = curl_easy_init();
    if(curl) {

        curl_easy_setopt(curl, CURLOPT_URL, downloadedJSON.URL.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        res = curl_easy_perform(curl);

        if(CURLE_OK == res) {
            char *ct;
            /* ask for the content-type */
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
            if((CURLE_OK == res) && ct) {
                downloadedJSON.response = response_string;
                downloadedJSON.failed = false;
                downloadedJSON.complete = true;
            }
        } else {
          downloadedJSON.response = "";
          downloadedJSON.failed = false;
          downloadedJSON.complete = false;
        }
    }
    /* always cleanup */
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    curl_global_cleanup();

}


static bool readBootstrapHash(boost::filesystem::path signaturePath, std::string &hash)
{
    std::ifstream signatureFile(signaturePath.string().c_str(), std::ifstream::binary);
    if (!signatureFile.good()) {
        return false;
    }

    std::string signatureJSON(
        (std::istreambuf_iterator<char>(signatureFile)),
        std::istreambuf_iterator<char>());
    UniValue signatureValue;
    if (!signatureValue.read(signatureJSON) || !signatureValue.isObject()) {
        return false;
    }

    const UniValue &hashValue = find_value(signatureValue, "hash");
    if (!hashValue.isStr()) {
        return false;
    }

    hash = hashValue.get_str();
    return hash.size() == 64 && IsHex(hash);
}


struct BootstrapCachePaths
{
    boost::filesystem::path manifest;
    boost::filesystem::path partial;
    boost::filesystem::path verified;
    boost::filesystem::path state;
};

struct BootstrapRemoteIdentity
{
    int64_t totalBytes;
    int64_t lastModified;
};

struct BootstrapResumeState
{
    std::string url;
    std::string expectedHash;
    int64_t totalBytes;
    int64_t lastModified;
    int64_t downloadedBytes;
    std::string phase;
};

static bool pathContains(
    const boost::filesystem::path &ancestor,
    const boost::filesystem::path &candidate)
{
    boost::filesystem::path::const_iterator left = ancestor.begin();
    boost::filesystem::path::const_iterator right = candidate.begin();
    for (; left != ancestor.end() && right != candidate.end(); ++left, ++right) {
        if (*left != *right) {
            return false;
        }
    }
    return left == ancestor.end();
}

static bool validBootstrapTransaction(const std::string &transaction)
{
    if (transaction.size() != 36) {
        return false;
    }
    for (size_t index = 0; index < transaction.size(); ++index) {
        const bool hyphen =
            index == 8 || index == 13 || index == 18 || index == 23;
        const unsigned char value =
            static_cast<unsigned char>(transaction[index]);
        if ((hyphen && value != '-') || (!hyphen && !std::isxdigit(value))) {
            return false;
        }
    }
    return true;
}

static bool bootstrapCachePaths(BootstrapCachePaths &paths)
{
    const std::string cacheArgument = GetArg("-bootstrapcachedir", "");
    const std::string transaction = GetArg("-bootstraptransaction", "");
    if (cacheArgument.empty() || !validBootstrapTransaction(transaction)) {
        LogPrintf("Bootstrap resume requires safe -bootstrapcachedir and -bootstraptransaction arguments\n");
        return false;
    }

    try {
        const boost::filesystem::path requested(cacheArgument);
        if (!requested.is_absolute() ||
            !boost::filesystem::exists(requested) ||
            !boost::filesystem::is_directory(requested) ||
            boost::filesystem::is_symlink(boost::filesystem::symlink_status(requested))) {
            return false;
        }
        const boost::filesystem::path cache = boost::filesystem::canonical(requested);
        const boost::filesystem::path data = boost::filesystem::canonical(GetDataDir());
        if (pathContains(cache, data) || pathContains(data, cache)) {
            LogPrintf("Bootstrap cache and data directories must be separate\n");
            return false;
        }
        const std::string prefix = ".verus-bootstrap-" + transaction;
        paths.manifest = cache / (prefix + ".manifest");
        paths.partial = cache / (prefix + ".archive.partial");
        paths.verified = cache / (prefix + ".archive.verified");
        paths.state = cache / (prefix + ".state.json");
        return true;
    } catch (const boost::filesystem::filesystem_error &error) {
        LogPrintf("Invalid Bootstrap cache: %s\n", error.what());
        return false;
    }
}

static void removeIfPresent(const boost::filesystem::path &path)
{
    try {
        if (boost::filesystem::symlink_status(path).type() !=
            boost::filesystem::file_not_found) {
            boost::filesystem::remove(path);
        }
    } catch (const boost::filesystem::filesystem_error &error) {
        LogPrintf("Unable to remove Bootstrap cache entry: %s\n", error.what());
    }
}

static bool safeRegularFile(const boost::filesystem::path &path)
{
    try {
        const boost::filesystem::file_status status =
            boost::filesystem::symlink_status(path);
        return boost::filesystem::is_regular_file(status) &&
            !boost::filesystem::is_symlink(status);
    } catch (const boost::filesystem::filesystem_error &) {
        return false;
    }
}

static void resetBootstrapDownload(const BootstrapCachePaths &paths)
{
    removeIfPresent(paths.partial);
    removeIfPresent(paths.state);
}

static bool atomicWriteBootstrapState(
    const BootstrapCachePaths &paths,
    const BootstrapResumeState &state)
{
    UniValue value(UniValue::VOBJ);
    value.pushKV("schemaVersion", 1);
    value.pushKV("url", state.url);
    value.pushKV("expectedHash", state.expectedHash);
    value.pushKV("totalBytes", state.totalBytes);
    value.pushKV("lastModified", state.lastModified);
    value.pushKV("downloadedBytes", state.downloadedBytes);
    value.pushKV("phase", state.phase);

    const boost::filesystem::path temporary(paths.state.string() + ".tmp");
    removeIfPresent(temporary);
    FILE *file = fopen(temporary.string().c_str(), "wb");
    if (!file) {
        return false;
    }
    const std::string encoded = value.write();
    const bool written =
        fwrite(encoded.data(), 1, encoded.size(), file) == encoded.size();
    if (written) {
        FileCommit(file);
    }
    const bool closed = fclose(file) == 0;
    if (!written || !closed || !RenameOver(temporary, paths.state)) {
        removeIfPresent(temporary);
        return false;
    }
    return true;
}

static bool readBootstrapState(
    const BootstrapCachePaths &paths,
    BootstrapResumeState &state)
{
    if (!safeRegularFile(paths.state)) {
        return false;
    }
    try {
        if (boost::filesystem::file_size(paths.state) > 16 * 1024) {
            return false;
        }
    } catch (const boost::filesystem::filesystem_error &) {
        return false;
    }
    std::ifstream file(paths.state.string().c_str(), std::ifstream::binary);
    if (!file.good()) {
        return false;
    }
    const std::string encoded(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    UniValue value;
    if (!value.read(encoded) || !value.isObject()) {
        return false;
    }
    const UniValue &schemaVersion = find_value(value, "schemaVersion");
    const UniValue &url = find_value(value, "url");
    const UniValue &expectedHash = find_value(value, "expectedHash");
    const UniValue &totalBytes = find_value(value, "totalBytes");
    const UniValue &lastModified = find_value(value, "lastModified");
    const UniValue &downloadedBytes = find_value(value, "downloadedBytes");
    const UniValue &phase = find_value(value, "phase");
    if (!schemaVersion.isNum() || schemaVersion.get_int() != 1 ||
        !url.isStr() || !expectedHash.isStr() ||
        !totalBytes.isNum() || !lastModified.isNum() ||
        !downloadedBytes.isNum() || !phase.isStr()) {
        return false;
    }
    state.url = url.get_str();
    state.expectedHash = expectedHash.get_str();
    state.totalBytes = totalBytes.get_int64();
    state.lastModified = lastModified.get_int64();
    state.downloadedBytes = downloadedBytes.get_int64();
    state.phase = phase.get_str();
    return state.expectedHash.size() == 64 && IsHex(state.expectedHash) &&
        state.totalBytes > 0 && state.lastModified >= 0 &&
        state.downloadedBytes >= 0 &&
        state.downloadedBytes <= state.totalBytes &&
        (state.phase == "partial" || state.phase == "verified");
}

static bool configureVerifiedHTTPS(CURL *curl, const std::string &url)
{
    return curl &&
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) == CURLE_OK &&
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L) == CURLE_OK &&
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L) == CURLE_OK &&
        curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS) == CURLE_OK &&
        curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS) == CURLE_OK &&
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L) == CURLE_OK &&
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L) == CURLE_OK &&
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L) == CURLE_OK &&
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L) == CURLE_OK;
}

static bool downloadBootstrapManifest(
    const std::string &url,
    const boost::filesystem::path &destination)
{
    const boost::filesystem::path temporary(destination.string() + ".tmp");
    removeIfPresent(temporary);
    FILE *file = fopen(temporary.string().c_str(), "wb");
    if (!file) {
        return false;
    }
    CURL *curl = curl_easy_init();
    bool success = configureVerifiedHTTPS(curl, url) &&
        curl_easy_setopt(
            curl, CURLOPT_MAXFILESIZE_LARGE,
            static_cast<curl_off_t>(1024 * 1024)) == CURLE_OK &&
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data) == CURLE_OK &&
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file) == CURLE_OK &&
        curl_easy_perform(curl) == CURLE_OK;
    long responseCode = 0;
    if (curl) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
        curl_easy_cleanup(curl);
    }
    if (success && responseCode == 200) {
        FileCommit(file);
    } else {
        success = false;
    }
    const bool closed = fclose(file) == 0;
    try {
        success = success &&
            boost::filesystem::file_size(temporary) <= 1024 * 1024;
    } catch (const boost::filesystem::filesystem_error &) {
        success = false;
    }
    if (!success || !closed || !RenameOver(temporary, destination)) {
        removeIfPresent(temporary);
        return false;
    }
    return true;
}

static bool probeBootstrapArchive(
    const std::string &url,
    BootstrapRemoteIdentity &identity)
{
    CURL *curl = curl_easy_init();
    if (!configureVerifiedHTTPS(curl, url)) {
        if (curl) curl_easy_cleanup(curl);
        return false;
    }
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);
    const CURLcode result = curl_easy_perform(curl);
    long responseCode = 0;
    long lastModified = -1;
    double contentLength = -1;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    curl_easy_getinfo(curl, CURLINFO_FILETIME, &lastModified);
    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLength);
    curl_easy_cleanup(curl);
    const double maximumArchiveBytes =
        static_cast<double>(1024LL * 1024LL * 1024LL * 1024LL);
    if (result != CURLE_OK || responseCode != 200 ||
        contentLength <= 0 || contentLength > maximumArchiveBytes ||
        lastModified < 0) {
        LogPrintf("Bootstrap server did not provide a stable resumable identity\n");
        return false;
    }
    identity.totalBytes = static_cast<int64_t>(contentLength);
    identity.lastModified = static_cast<int64_t>(lastModified);
    return identity.totalBytes > 0;
}

struct BootstrapTransferContext
{
    BootstrapCachePaths paths;
    BootstrapResumeState state;
    FILE *file;
    int64_t baseBytes;
    int64_t lastCheckpointTime;
    bool checkpointFailed;
};

struct BootstrapResponseHeaders
{
    bool contentRangeSeen;
    int64_t contentRangeStart;
    int64_t contentRangeEnd;
    int64_t contentRangeTotal;
};

static size_t bootstrapResponseHeader(
    char *buffer,
    size_t size,
    size_t count,
    void *opaque)
{
    const size_t byteCount = size * count;
    BootstrapResponseHeaders *headers =
        static_cast<BootstrapResponseHeaders *>(opaque);
    std::string line(buffer, byteCount);
    std::string lower(line);
    for (std::string::iterator it = lower.begin(); it != lower.end(); ++it) {
        *it = static_cast<char>(std::tolower(
            static_cast<unsigned char>(*it)));
    }
    const std::string prefix = "content-range:";
    if (lower.compare(0, prefix.size(), prefix) == 0) {
        long long start = -1;
        long long end = -1;
        long long total = -1;
        if (sscanf(
                line.substr(prefix.size()).c_str(),
                " bytes %lld-%lld/%lld",
                &start, &end, &total) == 3 &&
            start >= 0 && end >= start && total > end) {
            headers->contentRangeSeen = true;
            headers->contentRangeStart = static_cast<int64_t>(start);
            headers->contentRangeEnd = static_cast<int64_t>(end);
            headers->contentRangeTotal = static_cast<int64_t>(total);
        }
    }
    return byteCount;
}

static int bootstrapTransferProgress(
    void *opaque,
    curl_off_t downloadTotal,
    curl_off_t downloadedNow,
    curl_off_t uploadTotal,
    curl_off_t uploadedNow)
{
    BootstrapTransferContext *context =
        static_cast<BootstrapTransferContext *>(opaque);
    const int64_t downloaded =
        context->baseBytes + static_cast<int64_t>(downloadedNow);
    if (downloaded < context->baseBytes ||
        downloaded > context->state.totalBytes) {
        context->checkpointFailed = true;
        return 1;
    }
    const int64_t now = GetTime();
    if (now >= context->lastCheckpointTime + 2 ||
        downloaded == context->state.totalBytes || ShutdownRequested()) {
        FileCommit(context->file);
        context->state.downloadedBytes = downloaded;
        context->state.phase = "partial";
        if (!atomicWriteBootstrapState(context->paths, context->state)) {
            context->checkpointFailed = true;
            return 1;
        }
        context->lastCheckpointTime = now;
        LogPrintf("bootstrap transfer: %lld/%lld\n",
                  (long long)downloaded,
                  (long long)context->state.totalBytes);
    }
    return ShutdownRequested() ? 1 : 0;
}

enum BootstrapDownloadResult
{
    BOOTSTRAP_DOWNLOAD_COMPLETE,
    BOOTSTRAP_DOWNLOAD_RESTART,
    BOOTSTRAP_DOWNLOAD_INTERRUPTED
};

static BootstrapDownloadResult downloadBootstrapPartial(
    const std::string &url,
    const std::string &expectedHash,
    const BootstrapRemoteIdentity &remote,
    const BootstrapCachePaths &paths,
    int64_t baseBytes)
{
    const char *mode = baseBytes > 0 ? "ab" : "wb";
    FILE *file = fopen(paths.partial.string().c_str(), mode);
    if (!file) {
        return BOOTSTRAP_DOWNLOAD_INTERRUPTED;
    }
    BootstrapTransferContext context;
    context.paths = paths;
    context.state.url = url;
    context.state.expectedHash = expectedHash;
    context.state.totalBytes = remote.totalBytes;
    context.state.lastModified = remote.lastModified;
    context.state.downloadedBytes = baseBytes;
    context.state.phase = "partial";
    context.file = file;
    context.baseBytes = baseBytes;
    context.lastCheckpointTime = 0;
    context.checkpointFailed = false;
    BootstrapResponseHeaders responseHeaders;
    responseHeaders.contentRangeSeen = false;
    responseHeaders.contentRangeStart = -1;
    responseHeaders.contentRangeEnd = -1;
    responseHeaders.contentRangeTotal = -1;
    if (!atomicWriteBootstrapState(paths, context.state)) {
        fclose(file);
        return BOOTSTRAP_DOWNLOAD_INTERRUPTED;
    }

    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;
    std::string range;
    if (!configureVerifiedHTTPS(curl, url)) {
        if (curl) curl_easy_cleanup(curl);
        fclose(file);
        return BOOTSTRAP_DOWNLOAD_INTERRUPTED;
    }
    if (baseBytes > 0) {
        range = strprintf("%lld-", (long long)baseBytes);
        curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
        const std::string ifRange =
            "If-Range: " + DateTimeStrFormat(
                "%a, %d %b %Y %H:%M:%S GMT",
                remote.lastModified);
        headers = curl_slist_append(headers, ifRange.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, bootstrapResponseHeader);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaders);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, bootstrapTransferProgress);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &context);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    const CURLcode result = curl_easy_perform(curl);
    long responseCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    FileCommit(file);
    const bool closed = fclose(file) == 0;
    if (!closed || context.checkpointFailed) {
        return BOOTSTRAP_DOWNLOAD_INTERRUPTED;
    }
    if (baseBytes > 0 &&
        (responseCode != 206 || !responseHeaders.contentRangeSeen ||
         responseHeaders.contentRangeStart != baseBytes ||
         responseHeaders.contentRangeTotal != remote.totalBytes ||
         responseHeaders.contentRangeEnd + 1 != remote.totalBytes)) {
        LogPrintf("Bootstrap Range/If-Range validation failed; restarting safely\n");
        return BOOTSTRAP_DOWNLOAD_RESTART;
    }
    if (baseBytes == 0 && responseCode != 200) {
        return BOOTSTRAP_DOWNLOAD_RESTART;
    }

    int64_t persistedBytes = -1;
    try {
        persistedBytes = static_cast<int64_t>(
            boost::filesystem::file_size(paths.partial));
    } catch (const boost::filesystem::filesystem_error &) {
        return BOOTSTRAP_DOWNLOAD_INTERRUPTED;
    }
    context.state.downloadedBytes = persistedBytes;
    if (!atomicWriteBootstrapState(paths, context.state)) {
        return BOOTSTRAP_DOWNLOAD_INTERRUPTED;
    }
    if (result != CURLE_OK) {
        return BOOTSTRAP_DOWNLOAD_INTERRUPTED;
    }
    if (persistedBytes != remote.totalBytes) {
        return BOOTSTRAP_DOWNLOAD_INTERRUPTED;
    }
    return BOOTSTRAP_DOWNLOAD_COMPLETE;
}

static bool resumableBootstrapArchive(
    const std::string &url,
    const std::string &expectedHash,
    const BootstrapCachePaths &paths)
{
    if (safeRegularFile(paths.verified)) {
        if (CalcSha256(paths.verified.string()) == expectedHash) {
            return true;
        }
        removeIfPresent(paths.verified);
        resetBootstrapDownload(paths);
    } else {
        removeIfPresent(paths.verified);
    }

    BootstrapRemoteIdentity remote;
    if (!probeBootstrapArchive(url, remote)) {
        return false;
    }

    for (int attempt = 0; attempt < 2; ++attempt) {
        int64_t baseBytes = 0;
        BootstrapResumeState state;
        if (readBootstrapState(paths, state) &&
            state.phase == "partial" && state.url == url &&
            state.expectedHash == expectedHash &&
            state.totalBytes == remote.totalBytes &&
            state.lastModified == remote.lastModified &&
            safeRegularFile(paths.partial)) {
            try {
                const int64_t fileBytes = static_cast<int64_t>(
                    boost::filesystem::file_size(paths.partial));
                if (fileBytes == state.downloadedBytes &&
                    fileBytes <= remote.totalBytes) {
                    baseBytes = fileBytes;
                } else {
                    resetBootstrapDownload(paths);
                }
            } catch (const boost::filesystem::filesystem_error &) {
                resetBootstrapDownload(paths);
            }
        } else {
            resetBootstrapDownload(paths);
        }

        if (baseBytes == remote.totalBytes && baseBytes > 0) {
            if (CalcSha256(paths.partial.string()) == expectedHash) {
                removeIfPresent(paths.verified);
                if (!RenameOver(paths.partial, paths.verified)) {
                    return false;
                }
                state.url = url;
                state.expectedHash = expectedHash;
                state.totalBytes = remote.totalBytes;
                state.lastModified = remote.lastModified;
                state.downloadedBytes = remote.totalBytes;
                state.phase = "verified";
                return atomicWriteBootstrapState(paths, state);
            }
            resetBootstrapDownload(paths);
            baseBytes = 0;
        }

        const BootstrapDownloadResult result = downloadBootstrapPartial(
            url, expectedHash, remote, paths, baseBytes);
        if (result == BOOTSTRAP_DOWNLOAD_INTERRUPTED) {
            return false;
        }
        if (result == BOOTSTRAP_DOWNLOAD_RESTART) {
            resetBootstrapDownload(paths);
            continue;
        }
        if (CalcSha256(paths.partial.string()) != expectedHash) {
            resetBootstrapDownload(paths);
            if (baseBytes > 0) {
                continue;
            }
            return false;
        }
        removeIfPresent(paths.verified);
        if (!RenameOver(paths.partial, paths.verified)) {
            return false;
        }
        BootstrapResumeState verifiedState;
        verifiedState.url = url;
        verifiedState.expectedHash = expectedHash;
        verifiedState.totalBytes = remote.totalBytes;
        verifiedState.lastModified = remote.lastModified;
        verifiedState.downloadedBytes = remote.totalBytes;
        verifiedState.phase = "verified";
        return atomicWriteBootstrapState(paths, verifiedState);
    }
    return false;
}

static void cleanExtractedBootstrapData()
{
    static const char *entries[] = {
        "blocks", "chainstate", "indexes", "notarisations"
    };
    for (size_t index = 0; index < sizeof(entries) / sizeof(entries[0]); ++index) {
        boost::filesystem::remove_all(GetDataDir() / entries[index]);
    }
}


bool getBootstrap() {
    initalizeMapParamBootstrap();
    ParamFile bootstrap;
    ParamFile signature;
    for (std::map<std::string, ParamFile>::iterator it = mapParams.begin(); it != mapParams.end(); ++it) {
        if (it->second.name == "bootstrap") {
            bootstrap = it->second;
        } else if (it->second.name == "bootstrap-signature") {
            signature = it->second;
        }
    }

    const bool hasCache = !GetArg("-bootstrapcachedir", "").empty();
    const bool hasTransaction = !GetArg("-bootstraptransaction", "").empty();
    const bool resumable = hasCache && hasTransaction;
    bool dlsuccess = false;
    std::string expectedHash;
    BootstrapCachePaths cachePaths;

    if (hasCache != hasTransaction) {
        LogPrintf("Bootstrap cache and transaction arguments must be supplied together\n");
    } else if (resumable) {
        dlsuccess = bootstrapCachePaths(cachePaths) &&
            curl_global_init(CURL_GLOBAL_ALL) == CURLE_OK;
        if (dlsuccess) {
            dlsuccess = downloadBootstrapManifest(
                signature.URL, cachePaths.manifest) &&
                readBootstrapHash(cachePaths.manifest, expectedHash) &&
                resumableBootstrapArchive(
                    bootstrap.URL, expectedHash, cachePaths);
            curl_global_cleanup();
        }
        if (dlsuccess) {
            bootstrap.path = cachePaths.verified;
            signature.path = cachePaths.manifest;
        }
    } else {
        dlsuccess = downloadFiles("Bootstrap");
        if (dlsuccess) {
            dlsuccess =
                readBootstrapHash(signature.path, expectedHash) &&
                CalcSha256(bootstrap.path.string()) == expectedHash;
        }
    }

    // The official .verusid manifest binds the archive to one SHA-256 hash.
    // TLS authentication and this digest check must both pass before extract.
    if (!dlsuccess) {
        LogPrintf("Bootstrap manifest, archive, or resume validation failed\n");
    }
    if (dlsuccess) {
        // A prior process may have stopped part-way through extraction. Always
        // reset every allowlisted chain-data entry before using the retained,
        // digest-verified archive again.
        cleanExtractedBootstrapData();
        if (!extract(bootstrap.path)) {
            cleanExtractedBootstrapData();
            dlsuccess = false;
        }
    }
    if (!resumable) {
        removeIfPresent(bootstrap.path);
        removeIfPresent(signature.path);
    }

    return dlsuccess;
}


bool extract(boost::filesystem::path filename) {

    bool extractComplete = true;
	struct archive *a;
	struct archive *ext;
	struct archive_entry *entry;
	int r;

    int flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;
    flags |= ARCHIVE_EXTRACT_SECURE_NODOTDOT;
    flags |= ARCHIVE_EXTRACT_SECURE_SYMLINKS;

	a = archive_read_new();
	ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);

	if (archive_read_support_format_tar(a) != ARCHIVE_OK)
      extractComplete = false;

    if (archive_read_support_filter_gzip(a) != ARCHIVE_OK)
        extractComplete = false;

    r = archive_read_open_filename(a, filename.string().c_str(), 10240);
	if (r != ARCHIVE_OK) {
        LogPrintf("archive_read_open_filename() %s %d\n",archive_error_string(a), r);
        extractComplete = false;
    }

    if (extractComplete) {
        for (;;) {
            r = archive_read_next_header(a, &entry);
            if (r == ARCHIVE_EOF) {
                break;
            }
            if (r != ARCHIVE_OK) {
                LogPrintf("archive_read_next_header() %s %d\n",archive_error_string(a), r);
                extractComplete = false;
                break;
            }

            const char* currentFile = archive_entry_pathname(entry);
            boost::filesystem::path relativePath(currentFile ? currentFile : "");
            bool unsafePath = relativePath.empty() || relativePath.is_absolute();
            for (boost::filesystem::path::iterator it = relativePath.begin();
                 !unsafePath && it != relativePath.end(); ++it) {
                if (it->string() == "..") {
                    unsafePath = true;
                }
            }
            mode_t fileType = archive_entry_filetype(entry);
            if (unsafePath ||
                (fileType != AE_IFREG && fileType != AE_IFDIR) ||
                archive_entry_hardlink(entry) != NULL ||
                archive_entry_symlink(entry) != NULL) {
                LogPrintf("Bootstrap archive contains an unsafe entry\n");
                extractComplete = false;
                break;
            }

            std::string path = (GetDataDir() / relativePath).string();
            std::string uiMessage = "Extracting Bootstrap file ";
            uiMessage.append(currentFile);
            uiInterface.InitMessage(_(uiMessage.c_str()));
            archive_entry_set_pathname(entry, path.c_str());
            r = archive_write_header(ext, entry);
            if (r != ARCHIVE_OK) {
                LogPrintf("archive_write_header() %s %d\n",archive_error_string(ext), r);
                extractComplete = false;
                break;
            } else {
                copy_data(a, ext);
                r = archive_write_finish_entry(ext);
                if (r != ARCHIVE_OK) {
                    LogPrintf("archive_write_finish_entry() %s %d\n",archive_error_string(ext), r);
                    extractComplete = false;
                    break;
                }
            }
        }
    }

	archive_read_close(a);
	archive_read_free(a);

	archive_write_close(ext);
    archive_write_free(ext);

	return extractComplete;
}


static int copy_data(struct archive *ar, struct archive *aw) {
    int r;
    const void *buff;
    size_t size;
    int64_t offset;

    for (;;) {
        r = archive_read_data_block(ar, &buff, &size, &offset);

        if (r == ARCHIVE_EOF)
            return (ARCHIVE_OK);

        if (r != ARCHIVE_OK)
            return (r);

        r = archive_write_data_block(aw, buff, size, offset);
        if (r != ARCHIVE_OK) {
            LogPrintf("archive_write_data_block() %s\n",archive_error_string(aw));
            return (r);
        }
    }
}
