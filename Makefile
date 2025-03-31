CPPS =  config/utils.cpp \
		config/Config.cpp \
		config/VirtualServer.cpp config/Location.cpp \
		HttpMessage/HttpMessageParser.cpp \
		HttpMessage/Request/Request.cpp \
		HttpMessage/Response/Response.cpp \
		HttpMessage/Response/autoindex.cpp \
		HttpMessage/Response/MetaVariables.cpp \
		HttpConnection/HttpConnection.cpp \
		HttpConnection/http_utils.cpp \
		main.cpp

CGIS = cgi_bin/test.cpp cgi_bin/time.cpp cgi_bin/loop.cpp 

CGIS_POST = cgi_bin/upload.cpp cgi_bin/upload2.cpp 

COOKIE_DIR = Cookie
COOKIE = Cookie/userinfo.txt

OBJS = $(CPPS:.cpp=.o)

NAME = webserv
CGI_TARGETS := $(patsubst cgi_bin/%.cpp,cgi_bin/%.cgi,$(CGIS))
CGI_POST_TARGETS := $(patsubst cgi_bin/%.cpp,cgi_bin/%.cgi,$(CGIS_POST))

cgi/%.cgi: cgi_bin/%.cpp
	$(CXX) $(CXXFLAGS) $< -o $@
cgi_post/%.cgi: cgi_bin/%.cpp
	$(CXX) $(CXXFLAGS) cgi_bin/utils/login.cpp $< -o $@

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

ifdef DEBUG
CXXFLAGS += -fsanitize=address -g3 
endif

all: $(NAME) $(CGI_TARGETS) $(CGI_POST_TARGETS);

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	mkdir -p Cookie
	@touch $(COOKIE)

clean: 
	rm -rf $(OBJS) */*.dSYM
	rm -f cgi/*.cgi
	rm -f cgi_post/*.cgi
	rm -f upload/*

fclean: clean
	rm -rf $(NAME) $(COOKIE)
	@for file in $(CPPS); do \
		sed -i '' 's|^\([[:space:]]*\)std::cerr << DEBUG|\1// std::cerr << DEBUG|g' "$$file"; \
	done
	@echo "Debug lines commented out"

re: fclean all;

debug: fclean $(CPPS)
	@for file in $(CPPS); do \
    	sed -i '' 's|^\([[:space:]]*\)// std::cerr << DEBUG|\1std::cerr << DEBUG|g' "$$file"; \
	done
	@echo "Debug lines uncommented"
	make DEBUG=1 all
undebug: $(CPPS)
	@for file in $(CPPS); do \
		sed -i '' 's|^\([[:space:]]*\)std::cerr << DEBUG|\1// std::cerr << DEBUG|g' "$$file"; \
	done
	@echo "Debug lines commented out"
	make re

.PHONY: all clean fclean re debug undebug