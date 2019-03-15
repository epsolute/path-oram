FROM dbogatov/docker-images:nginx-latest

LABEL maintainer="dmytro@dbogatov.org"

WORKDIR /srv

# Copy the source
COPY docs/html .

# Copy the NGINX config
COPY nginx.conf /etc/nginx/conf.d/default.conf

CMD ["nginx", "-g", "daemon off;"]
