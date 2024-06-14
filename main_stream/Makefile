CPPS =  ../config/utils.cpp ../config/Config.cpp ../config/VirtualServer.cpp ../config/Location.cpp \
		../request/RequestParse.cpp \
<<<<<<< HEAD
		../HttpConnection/HttpConnection.cpp ../HttpConnection/HttpSession.cpp \
=======
		../HttpConnection/HttpConnection.cpp \
		../HttpConnection/autoindex.cpp \
		../HttpConnection/http_utils.cpp \
		../HttpConnection/post_delete_utils.cpp \
>>>>>>> 06e8c515c5f53e744c66e3f23e0993204ae9b134
		main.cpp

OBJS = ${CPPS:.cpp=.o}

NAME = webserv
CGI = test.cgi
CGI_POST = upload.cgi
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g3
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(CGI): ../cgi/test.cpp
	@$(CXX) $(CXXFLAGS) ../cgi/test.cpp -o $(CGI)

$(CGI_POST): ../cgi_post/upload.cpp
	@$(CXX) $(CXXFLAGS) ../cgi_post/upload.cpp -o $(CGI_POST)

.PHONY: all clean fclean re

all: $(NAME) $(CGI) $(CGI_POST)
	@mv $(CGI) ../cgi
	@mv $(CGI_POST) ../cgi_post

clean: 
	rm -rf $(OBJS)
	rm -rf test.cgi.dSYM
	rm -rf upload.cgi.dSYM

fclean: clean
	rm -rf $(NAME) ../cgi/$(CGI) ../cgi_post/$(CGI_POST)

re: fclean all;
