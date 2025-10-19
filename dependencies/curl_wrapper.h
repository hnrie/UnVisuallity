#pragma once

#include <iostream>
#include <string>

#include <nlohmann/json.hpp>
#include <curl/curl.h>

#include "xorstr/xorstr.h"

#undef GET
#undef POST
#undef PUT
#undef DELETE
#undef OPTIONS
#undef HEAD

inline size_t HeaderCallback(char* Buffer, size_t Size, size_t nItems, std::vector<std::string>* Headers) {
    size_t TotalSize = Size * nItems;
    std::string Header(Buffer, TotalSize);

    while (!Header.empty() && (Header.back() == '\r' || Header.back() == '\n'))
        Header.pop_back();

    if (!Header.empty())
        Headers->push_back(Header);

    return TotalSize;
}

class CCookies {
public:
    CCookies() = default;

    std::vector<std::string> Cookies;

    std::string Get() {
        std::string Result = "";

        for (const std::string& Cookie : Cookies)
            Result += Cookie + "; ";

        return Result;
    }

    void Insert(const std::string& Cookie) {
        Cookies.push_back(Cookie);
    }
};

class CHeaders {
public:
    CHeaders() = default;

    std::vector<std::string> Headers;

    std::string Get() {
        std::string Result = "";

        for (const auto& Header : Headers)
            Result += Header + "; ";

        return Result;
    }

    void Insert(const std::string& HeaderName, const std::string& HeaderValue) {
        Headers.push_back(HeaderName + ": " + HeaderValue);
    }
};

class CResponse {
public:
    CResponse() = default;

    std::string Res;
    int StatusCode;
    std::string StatusMessage;
    std::vector<std::string> Headers;

    std::string Get() { return Res; }

    nlohmann::json Json() {
        return nlohmann::json::parse(Res);
    }
};

class CCurlWrapper {
private:
    static std::string ParseStatusCode(int Status) {
        switch (Status) {
            case 100: return "Continue";
            case 101: return "Switching Protocols";
            case 102: return "Processing";
            case 103: return "Early Hints";
            case 200: return "OK";
            case 201: return "Created";
            case 202: return "Accepted";
            case 203: return "Non-Authoritative Information";
            case 204: return "No Content";
            case 205: return "Reset Content";
            case 206: return "Partial Content";
            case 207: return "Multi-Status";
            case 208: return "Already Reported";
            case 226: return "IM Used";
            case 300: return "Multiple Choices";
            case 301: return "Moved Permanently";
            case 302: return "Found";
            case 303: return "See Other";
            case 304: return "Not Modified";
            case 305: return "Use Proxy";
            case 306: return "Switch Proxy";
            case 307: return "Temporary Redirect";
            case 308: return "Permanent Redirect";
            case 400: return "Bad Request";
            case 401: return "Unauthorized";
            case 402: return "Payment Required";
            case 403: return "Forbidden";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            case 406: return "Not Acceptable";
            case 407: return "Proxy Authentication Required";
            case 408: return "Request Timeout";
            case 409: return "Conflict";
            case 410: return "Gone";
            case 411: return "Length Required";
            case 412: return "Precondition Failed";
            case 413: return "Payload Too Large";
            case 414: return "URI Too Long";
            case 415: return "Unsupported Media Type";
            case 416: return "Range Not Satisfiable";
            case 417: return "Expectation Failed";
            case 418: return "I'm a teapot";
            case 421: return "Misdirected Request";
            case 422: return "Unprocessable Entity";
            case 423: return "Locked";
            case 424: return "Failed Dependency";
            case 425: return "Too Early";
            case 426: return "Upgrade Required";
            case 428: return "Precondition Required";
            case 429: return "Too Many Requests";
            case 431: return "Request Header Fields Too Large";
            case 451: return "Unavailable For Legal Reasons";
            case 500: return "Internal Server Error";
            case 501: return "Not Implemented";
            case 502: return "Bad Gateway";
            case 503: return "Service Unavailable";
            case 504: return "Gateway Timeout";
            case 505: return "HTTP Version Not Supported";
            case 506: return "Variant Also Negotiates";
            case 507: return "Insufficient Storage";
            case 508: return "Loop Detected";
            case 510: return "Not Extended";
            case 511: return "Network Authentication Required";
            default: return "Unknown";
        };
    }

    static size_t CurlWriteCallback(void* Contents, size_t Size, size_t nMemb, std::string* s) {
        size_t NewLength = Size * nMemb;
        size_t OldLength = s->size();
        try {
            s->resize(OldLength + NewLength);
        }
        catch (std::bad_alloc& e) {
            return 0;
        }

        std::copy((char*)Contents, (char*)Contents + NewLength, s->begin() + OldLength);

        return Size * nMemb;
    }
public:
    CResponse GET(const std::string& Url, const CCookies& _Cookies = CCookies(), const CHeaders& _Headers = CHeaders()) {
        CResponse Response;

        CURL* Curl = curl_easy_init();
        if (Curl) {
            curl_easy_setopt(Curl, CURLOPT_URL, Url.c_str());
            curl_easy_setopt(Curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
            curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &Response.Res);
            curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYHOST, 0L);

            struct curl_slist* Cookies = NULL;
            for (auto& Cookie : _Cookies.Cookies)
                Cookies = curl_slist_append(Cookies, Cookie.c_str());

            curl_easy_setopt(Curl, CURLOPT_COOKIE, Cookies);

            struct curl_slist* Headers = NULL;
            for (auto& Header : _Headers.Headers)
                Headers = curl_slist_append(Headers, Header.c_str());

            curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, Headers);

            FILE* TempFile = tmpfile();
            if (!TempFile) {
                Response.StatusCode = -1;
                Response.StatusMessage = OBF("Failed to create temporary file");
                curl_easy_cleanup(Curl);
                return Response;
            }

            curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, NULL);
            curl_easy_setopt(Curl, CURLOPT_WRITEDATA, TempFile);

            curl_easy_setopt(Curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
            curl_easy_setopt(Curl, CURLOPT_HEADERDATA, &Response.Headers);

            CURLcode res = curl_easy_perform(Curl);
            if (res != CURLE_OK) {
                Response.StatusCode = -1;
                Response.StatusMessage = OBF("CURL request failed: ") + std::string(curl_easy_strerror(res));
            }
            else {
                curl_easy_getinfo(Curl, CURLINFO_RESPONSE_CODE, &Response.StatusCode);

                rewind(TempFile);

                std::string FileContent;
                char Buffer[1024];
                while (size_t Len = fread(Buffer, 1, sizeof(Buffer), TempFile))
                    FileContent.append(Buffer, Len);

                Response.Res = FileContent;
                Response.StatusMessage = ParseStatusCode(Response.StatusCode);
            }

            fclose(TempFile);
            curl_easy_cleanup(Curl);
        }
        else {
            Response.StatusCode = -1;
            Response.StatusMessage = OBF("Failed to initialize curl");
        }

        return Response;
    }

CResponse POST(const std::string& Url, const nlohmann::json& Body, const CCookies& _Cookies = CCookies(), const CHeaders& _Headers = CHeaders()) {
    CResponse Response;

    CURL* Curl = curl_easy_init();
    if (!Curl) {
        Response.StatusCode = -1;
        Response.StatusMessage = OBF("Failed to initialize curl");
        return Response;
    }

    curl_easy_setopt(Curl, CURLOPT_URL, Url.c_str());
    curl_easy_setopt(Curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &Response.Res);
    curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // Add Cookies as a single string
    if (!_Cookies.Cookies.empty()) {
        std::string cookieStr;
        for (const auto& cookie : _Cookies.Cookies) {
            cookieStr += cookie + "; ";
        }
        if (!cookieStr.empty()) {
            cookieStr.pop_back(); // remove trailing space
            curl_easy_setopt(Curl, CURLOPT_COOKIE, cookieStr.c_str());
        }
    }

    // Add Headers
    struct curl_slist* Headers = NULL;
    bool hasContentType = false;
    for (const auto& header : _Headers.Headers) {
        Headers = curl_slist_append(Headers, header.c_str());
        if (header.find(OBF("Content-Type:")) != std::string::npos)
            hasContentType = true;
    }

    if (!hasContentType) {
        Headers = curl_slist_append(Headers, OBF("Content-Type: application/json"));
    }

    curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, Headers);

    std::string json_payload = Body.dump();
    curl_easy_setopt(Curl, CURLOPT_POSTFIELDS, json_payload.c_str());

    curl_easy_setopt(Curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(Curl, CURLOPT_HEADERDATA, &Response.Headers);

    CURLcode res = curl_easy_perform(Curl);
    if (res != CURLE_OK) {
        Response.StatusMessage = curl_easy_strerror(res);
    }

    curl_easy_getinfo(Curl, CURLINFO_RESPONSE_CODE, &Response.StatusCode);
    if (Response.StatusMessage.empty()) {
        Response.StatusMessage = ParseStatusCode(Response.StatusCode);
    }

    // Clean up
    curl_easy_cleanup(Curl);
    if (Headers)
        curl_slist_free_all(Headers);

    return Response;
}


    CResponse POST_NOT_JSON(const std::string& Url, const std::string& Body, const CCookies& _Cookies, const CHeaders& _Headers) {
        CResponse Response;

        CURL* Curl = curl_easy_init();
        if (Curl) {
            curl_easy_setopt(Curl, CURLOPT_URL, Url.c_str());
            curl_easy_setopt(Curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
            curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &Response.Res);
            curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYHOST, 0L);

            struct curl_slist* Cookies = NULL;
            for (auto& Cookie : _Cookies.Cookies)
                Cookies = curl_slist_append(Cookies, Cookie.c_str());

            curl_easy_setopt(Curl, CURLOPT_COOKIE, Cookies);

            struct curl_slist* Headers = NULL;
            for (auto& Header : _Headers.Headers)
                Headers = curl_slist_append(Headers, Header.c_str());

            curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, Headers);

            curl_easy_setopt(Curl, CURLOPT_POSTFIELDS, Body.data());

            curl_easy_setopt(Curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
            curl_easy_setopt(Curl, CURLOPT_HEADERDATA, &Response.Headers);

            CURLcode res = curl_easy_perform(Curl);

            curl_easy_getinfo(Curl, CURLINFO_RESPONSE_CODE, &Response.StatusCode);

            Response.StatusMessage = ParseStatusCode(Response.StatusCode);

            curl_easy_cleanup(Curl);
            if (Cookies)
                curl_slist_free_all(Cookies);

            if (Headers)
                curl_slist_free_all(Headers);
        }
        else {
            Response.StatusCode = -1;
            Response.StatusMessage = OBF("Failed to initialize curl");
        }

        return Response;
    }

    CResponse PUT(const std::string& Url, const CCookies& _Cookies = CCookies(), const CHeaders& _Headers = CHeaders()) {
        CResponse Response;

        CURL* Curl = curl_easy_init();
        if (Curl) {
            curl_easy_setopt(Curl, CURLOPT_URL, Url.c_str());
            curl_easy_setopt(Curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
            curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &Response.Res);
            curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYHOST, 0L);

            struct curl_slist* Cookies = NULL;
            for (auto& Cookie : _Cookies.Cookies)
                Cookies = curl_slist_append(Cookies, Cookie.c_str());

            curl_easy_setopt(Curl, CURLOPT_COOKIE, Cookies);

            struct curl_slist* Headers = NULL;
            for (auto& Header : _Headers.Headers)
                Headers = curl_slist_append(Headers, Header.c_str());

            curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, Headers);

            curl_easy_setopt(Curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
            curl_easy_setopt(Curl, CURLOPT_HEADERDATA, &Response.Headers);

            CURLcode res = curl_easy_perform(Curl);

            curl_easy_getinfo(Curl, CURLINFO_RESPONSE_CODE, &Response.StatusCode);
            Response.StatusMessage = ParseStatusCode(Response.StatusCode);

            curl_easy_cleanup(Curl);

            if (Cookies)
                curl_slist_free_all(Cookies);

            if (Headers)
                curl_slist_free_all(Headers);
        }
        else {
            Response.StatusCode = -1;
            Response.StatusMessage = OBF("Failed to initialize curl");
        }

        return Response;
    }

    CResponse DELETE(const std::string& Url, const CCookies& _Cookies = CCookies(), const CHeaders& _Headers = CHeaders()) {
        CResponse Response;

        CURL* Curl = curl_easy_init();
        if (Curl) {
            curl_easy_setopt(Curl, CURLOPT_URL, Url.c_str());
            curl_easy_setopt(Curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
            curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &Response.Res);
            curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYHOST, 0L);

            struct curl_slist* Cookies = NULL;
            for (auto& Cookie : _Cookies.Cookies)
                Cookies = curl_slist_append(Cookies, Cookie.c_str());

            curl_easy_setopt(Curl, CURLOPT_COOKIE, Cookies);

            struct curl_slist* Headers = NULL;
            for (auto& Header : _Headers.Headers)
                Headers = curl_slist_append(Headers, Header.c_str());

            curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, Headers);

            curl_easy_setopt(Curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
            curl_easy_setopt(Curl, CURLOPT_HEADERDATA, &Response.Headers);

            CURLcode res = curl_easy_perform(Curl);

            curl_easy_getinfo(Curl, CURLINFO_RESPONSE_CODE, &Response.StatusCode);
            Response.StatusMessage = ParseStatusCode(Response.StatusCode);

            curl_easy_cleanup(Curl);
            if (Cookies)
                curl_slist_free_all(Cookies);

            if (Headers)
                curl_slist_free_all(Headers);
        }
        else {
            Response.StatusCode = -1;
            Response.StatusMessage = OBF("Failed to initialize curl");
        }

        return Response;
    }

    CResponse OPTIONS(const std::string& Url, const CCookies& _Cookies = CCookies(), const CHeaders& _Headers = CHeaders()) {
        CResponse Response;

        CURL* Curl = curl_easy_init();
        if (Curl) {
            curl_easy_setopt(Curl, CURLOPT_URL, Url.c_str());
            curl_easy_setopt(Curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
            curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
            curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &Response.Res);
            curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYHOST, 0L);

            struct curl_slist* Cookies = NULL;
            for (auto& Cookie : _Cookies.Cookies)
                Cookies = curl_slist_append(Cookies, Cookie.c_str());

            curl_easy_setopt(Curl, CURLOPT_COOKIE, Cookies);

            struct curl_slist* Headers = NULL;
            for (auto& Header : _Headers.Headers)
                Headers = curl_slist_append(Headers, Header.c_str());

            curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, Headers);

            curl_easy_setopt(Curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
            curl_easy_setopt(Curl, CURLOPT_HEADERDATA, &Response.Headers);

            CURLcode res = curl_easy_perform(Curl);

            curl_easy_getinfo(Curl, CURLINFO_RESPONSE_CODE, &Response.StatusCode);
            Response.StatusMessage = ParseStatusCode(Response.StatusCode);

            curl_easy_cleanup(Curl);

            if (Cookies)
                curl_slist_free_all(Cookies);

            if (Headers)
                curl_slist_free_all(Headers);
        }
        else {
            Response.StatusCode = -1;
            Response.StatusMessage = OBF("Failed to initialize curl");
        }

        return Response;
    }

    CResponse HEAD(const std::string& Url, const CCookies& _Cookies = CCookies(), const CHeaders& _Headers = CHeaders()) {
        CResponse Response;

        CURL* Curl = curl_easy_init();
        if (Curl) {
            curl_easy_setopt(Curl, CURLOPT_URL, Url.c_str());
            curl_easy_setopt(Curl, CURLOPT_CUSTOMREQUEST, "HEAD");
            curl_easy_setopt(Curl, CURLOPT_NOBODY, 1L);
            curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
            curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &Response.Res);
            curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYHOST, 0L);

            struct curl_slist* Cookies = NULL;
            for (auto& Cookie : _Cookies.Cookies)
                Cookies = curl_slist_append(Cookies, Cookie.c_str());

            curl_easy_setopt(Curl, CURLOPT_COOKIE, Cookies);

            struct curl_slist* Headers = NULL;
            for (auto& header : _Headers.Headers)
                Headers = curl_slist_append(Headers, header.c_str());

            curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, Headers);

            curl_easy_setopt(Curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
            curl_easy_setopt(Curl, CURLOPT_HEADERDATA, &Response.Headers);

            CURLcode res = curl_easy_perform(Curl);

            curl_easy_getinfo(Curl, CURLINFO_RESPONSE_CODE, &Response.StatusCode);
            Response.StatusMessage = ParseStatusCode(Response.StatusCode);

            curl_easy_cleanup(Curl);

            if (Cookies)
                curl_slist_free_all(Cookies);

            if (Headers)
                curl_slist_free_all(Headers);
        }
        else {
            Response.StatusCode = -1;
            Response.StatusMessage = OBF("Failed to initialize curl");
        }

        return Response;
    }
};

inline auto g_curl_wrapper = std::make_unique<CCurlWrapper>();