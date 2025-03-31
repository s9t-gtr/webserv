#ifndef HTTPMESSAGEPARSER_HPP
# define HTTPMESSAGEPARSER_HPP

#include <exception>
#include <map>
#include <sstream>
#include <string>

#define StatusCode_t unsigned int

#define CRLF "\r\n"
#define CRLF_LEN 2
#define STR_LF "\n"
#define STR_CR "\r"
#define CHAR_LF '\n'
#define CHAR_CR '\r'

typedef std::map<std::string, std::string> HeadersMap;

enum ChunkedStatus{
	ChunkSize,
	ChunkData,
	ChunkCRLF,
	ChunkTrailer,
	ChunkEndCRLF
};

class HttpMessageParser{
    public:
        enum ReadingStatus{
            StartLine,
            Headers,
            Body,
            Complete
        };
        
        
        union DetailStatus{
            union StartLineReadStatus{
                enum RequestLineReadStatus{
                    SpaceBeforeMethod,
                    Method,
                    SpaceBeforeRequestTarget,
                    RequestTarget,
                    SpaceBeforeVersion,
                    Version,
                    TrailingSpaces,
                };
                enum StatusLineReadStatus{
                    SpaceBeforeHttpVersion,
                    HttpVersion,
                    SpaceBeforeStatusCode,
                    StatusCode,
                    SpaceBeforeReasonPhrase,
                    ReasonPhrase,
                };
                RequestLineReadStatus requestLineStatus;
                StatusLineReadStatus statusLineStatus;
            };
            enum HeaderReadStatus{
                FirstLineForbiddenSpace,
                SpaceBeforeFieldName,
                FieldName,
                SpaceBeforeFieldValue,
                FieldValue,
                SpaceBeforeCrlf,
                Crlf
            };
            enum BodyReadStatus{
                Nomal,

            };
            StartLineReadStatus startLineStatus;
            HeaderReadStatus headerStatus;
            BodyReadStatus bodyStatus;
        };
    public:
        HttpMessageParser(DetailStatus::StartLineReadStatus startLineReadStatus);
        virtual ~HttpMessageParser();

        void readBufferAndParse(char *buffer, size_t bytesReceived);

        virtual void parseStartLine(char c) = 0;
        void parseHeaders(char c);
        void parseHeaderContentLength();
        void parseBody(char c);

		void bodyUnChunk(char c);
        
        virtual void checkIsWatingLF(char c, bool isExistThirdElement) = 0;

        void toTheNextStatus();

        ReadingStatus getReadingStatus();
        void setReadingStatus(ReadingStatus readingStatus);

        DetailStatus getDetailStatus();
        void setDetailStatus(DetailStatus::StartLineReadStatus::RequestLineReadStatus newDetailStatus);
        void setDetailStatus(DetailStatus::StartLineReadStatus::StatusLineReadStatus newDetailStatus);
        void setDetailStatus(DetailStatus::HeaderReadStatus newDetailStatus);
        void setDetailStatus(DetailStatus::BodyReadStatus newDetailStatus);

        bool getIsWatingLF();
        void setIsWatingLF(bool value);

        bool getIsAfterFieldLineCRLF();
        void setIsAfterFieldLineCRLF(bool value);

        bool isOnlyAsciiCharacter(std::string line);

        std::string getField(std::string fieldName);
        std::string getBody();

        void syntaxErrorDetected(StatusCode_t statusCode = 400);
        bool getSyntaxErrorDetected();
        unsigned int getResponseStatusCode();
        std::string::size_type getContentLength();
    private:
        bool isWatingLF;
        bool isAfterFieldLineCRLF;
        ReadingStatus readingStatus;
        DetailStatus detailStatus;

        bool isSyntaxErrorDetected;

        HeadersMap headers;
        std::string body;
        std::string::size_type contentLength;

        unsigned int responseStatusCode;

        ChunkedStatus chunkedStatus;
        std::string chunkSizeString;
        long long chunkSize;
        bool isAfterSize;
};

class BadRequest_400: public std::invalid_argument{
    public:
        BadRequest_400(const char* error_message);
        const char* what() const throw();
};

class Found_302: public std::invalid_argument{
    public:
        Found_302(const char* error_message);
        const char* what() const throw();
};

class NotAllowed_405: public std::invalid_argument{
    public:
        NotAllowed_405(const char* error_message);
        const char* what() const throw();
};

class BadCgiResponse: public std::invalid_argument{
    public:
        BadCgiResponse(const char* error_message);
        const char* what() const throw();
};

class BadGateway_502: public std::invalid_argument{
    public:
        BadGateway_502(const char* error_message);
        const char* what() const throw();
};


#endif
