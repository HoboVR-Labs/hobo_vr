// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#pragma once

#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <iterator>
#include <regex>
#include <string>
#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>

namespace recvv {
  //can receive packets ending with \t\r\n using either winsock2 or unix sockets
  template <typename T>
  inline int receive_till_zero( T sock, void* buf, int& numbytes, int max_packet_size )
  {
    // receives a message until an end token is reached
    // thanks to https://stackoverflow.com/a/13528453/10190971
    int i = 0;
    int n=-1;
    do {
      // Check if we have a complete message
      for( ; i < numbytes-2; i++ ) {
        if(memcmp((char*)buf + i, "\t\r\n", 3) == 0) {
          return i + 3; // return length of message
        }
      }

      n = recv( sock, (char*)buf + numbytes, max_packet_size - numbytes, 0 );

      if( n == -1 ) {
        return -1; // operation failed!
      }
      numbytes += n;
    } while( true );
  }

  // reshapes packet vector into shape inxs
  template <typename T>
  inline std::vector<std::vector<T>> split_pk(std::vector<T> arr, std::vector<int> inxs) {
    std::vector<std::vector<T>> out;
    int oof = 0;
    int ms = (int)arr.size();
    for (int i : inxs) {
      if (oof + i <= ms) {
        std::vector<T> temp(arr.begin() + oof, arr.begin() + oof + i);
        out.push_back(temp);
        oof += i;
      }
      else {
        std::vector<T> temp(arr.begin() + oof, arr.end());
        out.push_back(temp);
        break;
      }

    }
    return out;
  }

  // get regex vector
  inline std::vector<std::string> get_rgx_vector(std::string ss, std::regex rgx) {
    std::sregex_token_iterator iter(ss.begin(),
        ss.end(),
        rgx,
        0);
    std::sregex_token_iterator end;

    std::vector<std::string> out;
    for (; iter != end; ++iter)
      out.push_back((std::string)*iter);
    return out;
  }

  // convert vector of strings to type
  template <typename T>
  inline std::vector<T> split_to_number(std::vector<std::string> split)
  {
    // converts a vector of strings to a vector of doubles
    std::vector<T> out(split.size());
    try {
      std::transform(split.begin(), split.end(), out.begin(), [](const std::string& val)
          {
            return (T)std::stod(val);
          });
    }
    catch (...) {
      out = { (T)0, (T)0 };
    }

    return out;
  }

  // something socket related idk
  inline void remove_message_from_buffer( char* buf, int& numbytes, int msglen )
  {
    // remove complete message from the buffer.
    // thanks to https://stackoverflow.com/a/13528453/10190971
    memmove( buf, buf + msglen, numbytes - msglen );
    numbytes -= msglen;
  }


  class Callback {
  public:
    virtual void OnPacket(char* buff, int len) = 0;
  };
}

#endif // UTIL_H