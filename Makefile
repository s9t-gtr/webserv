CPPS =  config/utils.cpp \
		config/Config.cpp \
		config/VirtualServer.cpp config/Location.cpp \
		request/RequestParse.cpp \
		HttpConnection/HttpConnection.cpp \
		HttpConnection/autoindex.cpp \
		HttpConnection/http_utils.cpp \
		HttpConnection/post_delete_utils.cpp \
		main.cpp

OBJS = ${CPPS:.cpp=.o}

NAME = webserv
CGI = test.cgi
CGI2 = time.cgi
CGI3 = loop.cgi
CGI_POST = upload.cgi
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

ifdef DEBUG
CXXFLAGS += -fsanitize=address -g3 
endif


all: $(NAME) $(CGI) $(CGI2) $(CGI3) $(CGI_POST)
	@mv $(CGI) cgi
	@mv $(CGI2) cgi
	@mv $(CGI3) cgi
	@mv $(CGI_POST) cgi_post

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(CGI): cgi/test.cpp
	@$(CXX) $(CXXFLAGS) cgi/test.cpp -o $(CGI)

$(CGI2): cgi/time.cpp
	@$(CXX) $(CXXFLAGS) cgi/time.cpp -o $(CGI2)

$(CGI3): cgi/loop.cpp
	@$(CXX) $(CXXFLAGS) cgi/loop.cpp -o $(CGI3)

$(CGI_POST): cgi_post/upload.cpp
	@$(CXX) $(CXXFLAGS) cgi_post/upload.cpp -o $(CGI_POST)

clean: 
	rm -rf $(OBJS) *.dSYM

fclean: clean
	rm -rf $(NAME) cgi/$(CGI) cgi/$(CGI2) cgi/$(CGI3) cgi_post/$(CGI_POST)

re: fclean all;

debug: fclean
	make DEBUG=1 all

.PHONY: all clean fclean re debug