CPPS =  config/utils.cpp \
		config/Config.cpp \
		config/VirtualServer.cpp config/Location.cpp \
		request/RequestParse.cpp \
		HttpConnection/HttpConnection.cpp \
		HttpConnection/autoindex.cpp \
		HttpConnection/http_utils.cpp \
		HttpConnection/post_delete_utils.cpp \
		main.cpp

CGIS = cgi/test.cpp cgi/time.cpp cgi/loop.cpp 

CGIS_POST = cgi_post/upload.cpp cgi_post/upload2.cpp 

OBJS = $(CPPS:.cpp=.o)

NAME = webserv
CGI_TARGETS := $(patsubst cgi/%.cpp,cgi/%.cgi,$(CGIS))
CGI_POST_TARGETS := $(patsubst cgi_post/%.cpp,cgi_post/%.cgi,$(CGIS_POST))

cgi/%.cgi: cgi/%.cpp
	$(CXX) $(CXXFLAGS) $< -o $@
cgi_post/%.cgi: cgi_post/%.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

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

clean: 
	rm -rf $(OBJS) */*.dSYM
	rm -f cgi/*.cgi
	rm -f cgi_post/*.cgi
	rm -f upload/*

fclean: clean
	rm -rf $(NAME)

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