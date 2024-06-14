FROM debian:bullseye
RUN apt update && \
	apt install -y vim nginx
#COPY ./tools/nginx_conf/nginx.conf /etc/nginx/nginx.conf

CMD ["nginx", "-g", "daemon off;"]
