#!/bin/bash
#
# Copyright (c) 2019-2024 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

set -e

repo_base=$(realpath $(dirname $0)/../..)

BK=cmake
IMAGE=build-cmake3_8
SHA=252732b3d7af7f78618e877479b85d4d611a61f4
CONTAINER=builder-$IMAGE
FULL_IMAGE=ghcr.io/anarthal-containers/$IMAGE:$SHA
DB=mysql8

docker start $DB || docker run -d \
    --name $DB \
    -v /var/run/mysqld:/var/run/mysqld \
    -p 3306:3306 \
    ghcr.io/anarthal-containers/$DB:$SHA
docker start $CONTAINER || docker run -dit \
    --name $CONTAINER \
    -v "$repo_base:/opt/boost-mysql" \
    -v /var/run/mysqld:/var/run/mysqld \
    $FULL_IMAGE
docker network connect my-net $DB || echo "DB already connected"
docker network connect my-net $CONTAINER || echo "Network already connected"

# Command line
db_args="--server-host=$DB --db=$DB"
case $BK in
    b2) cmd="$db_args
            --toolset=gcc
            --cxxstd=20
            --variant=debug
            --stdlib=native
            --address-model=64
            --separate-compilation=1
            --use-ts-executor=0
            --address-sanitizer=0
            --undefined-sanitizer=0
            --coverage=0
            --valgrind=0"
        ;;
    
    cmake) cmd="$db_args
            --cmake-build-type=Debug
            --build-shared-libs=1
            --cxxstd=20
            --install-test=1
            "
        ;;
    
    fuzz) cmd="$db_args" ;;

    *) cmd="" ;;
esac

# Run
docker exec $CONTAINER python /opt/boost-mysql/tools/ci/main.py --source-dir=/opt/boost-mysql $BK $cmd
