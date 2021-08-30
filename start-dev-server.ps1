docker run -p 3000:3000 --mount src=$pwd/server,target=/usr/src/app,type=bind -ti bsq-crash-reporter $args
