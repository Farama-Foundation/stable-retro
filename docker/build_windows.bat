cd ..
docker build . --tag farama-foundation/stable-retro:win64-v2 --file docker/windows/Dockerfile.build --progress plain

docker run -i -t farama-foundation/stable-retro:win64-v2 /bin/bash

REM Example of how you would extract the data
REM docker cp 915ff70ebc56:/root/stable-retro/dist/gym_retro-0.9.0-cp310-cp310-win_amd64.whl .
REM pip install retro/dist/gym_retro-0.9.0-cp310-cp310-win_amd64.whl

