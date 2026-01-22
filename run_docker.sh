# #!/bin/bash

# # ホストのXサーバーへのアクセスを許可
# xhost +local:docker

# docker build -t gomoku-dev .

# # GUIを表示するために DISPLAY 環境変数と X11 のソケットを共有
# docker run -it --rm \
#     -e DISPLAY=$DISPLAY \
#     -v /tmp/.X11-unix:/tmp/.X11-unix \
#     -v $(pwd):/app \
#     gomoku-dev /bin/bash

docker build -t gomoku-dev .

docker run -it --rm \
    -p 8080:8080 \
    -v $(pwd):/app \
    gomoku-dev
