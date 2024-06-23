CPPS =  config/utils.cpp config/Config.cpp config/VirtualServer.cpp config/Location.cpp \
		request/RequestParse.cpp \
		HttpConnection/HttpConnection.cpp \
		HttpConnection/autoindex.cpp \
		HttpConnection/http_utils.cpp \
		HttpConnection/post_delete_utils.cpp \
		main.cpp

OBJS = ${CPPS:.cpp=.o}

NAME = webserv
CGI = test1.cgi
CGI2 = test2.cgi
CGI_POST = upload.cgi
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

ifdef DEBUG
CXXFLAGS += -g3
endif


all: $(NAME) $(CGI) $(CGI2) $(CGI_POST)
	@mv $(CGI) cgi
	@mv $(CGI2) cgi
	@mv $(CGI_POST) cgi_post

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(CGI): cgi/test1.cpp
	@$(CXX) $(CXXFLAGS) cgi/test1.cpp -o $(CGI)

$(CGI2): cgi/test2.cpp
	@$(CXX) $(CXXFLAGS) cgi/test2.cpp -o $(CGI2)

$(CGI_POST): cgi_post/upload.cpp
	@$(CXX) $(CXXFLAGS) cgi_post/upload.cpp -o $(CGI_POST)

clean: 
	rm -rf $(OBJS)
	@rm -rf test1.cgi.dSYM
	@rm -rf test2.cgi.dSYM
	@rm -rf upload.cgi.dSYM

fclean: clean
	rm -rf $(NAME) cgi/$(CGI) cgi/$(CGI2) cgi_post/$(CGI_POST)

re: fclean all;
debug:
	$(MAKE) DEBUG=1 all

.PHONY: all clean fclean re debug