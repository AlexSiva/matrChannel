#include <ctime>
#include <iostream>
#include <thread>
#include <vector>

#include "channel.h"

int** makeMatrix(int n) {
  int** matrix = new int*[n];
  for (int i = 0; i < n; ++i) {
    matrix[i] = new int[n];
    for (int j = 0; j < n; ++j) {
      matrix[i][j] = rand() % 10;
    }
  }
  return matrix;
}

void printMatrix(int** a, int n) {
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      std::cout << a[i][j] << ' ';
    }
    std::cout << '\n';
  }
  std::cout << '\n';
}

void blocksMult(int** a, int** b, int** c, int n, int block,
                std::pair<int, int>& pair) {
  for (int i = pair.first; i < std::min(n, pair.first + block); ++i) {
    for (int j = pair.second; j < std::min(n, pair.second + block); ++j) {
      c[i][j] = 0;
      for (int k = 0; k < n; ++k) {
        c[i][j] += a[i][k] * b[k][j];
      }
    }
  }
}
void multChannel(int** a, int** b, int** c, int n, int block,
                 Channel<std::pair<int, int>>& channel) {
  std::pair<std::pair<int, int>, bool> pair = channel.Recv();
  while (pair.second) {
    blocksMult(a, b, c, n, block, pair.first);
    pair = channel.Recv();
  }
}
void multAll(int** a, int** b, int** c, int n, int block, int thrCount) {
  std::vector<std::thread> threads;
  int perem = n % block == 0 ? n / block : n / block + 1;
  Channel<std::pair<int, int>> channel(perem * perem);
  for (int I = 0; I < n; I += block) {
    for (int J = 0; J < n; J += block) {
      std::pair<int, int> pair(I, J);
      channel.Send(std::move(pair));
    }
  }
  channel.Close();
  for (int i = 0; i < thrCount; ++i) {
    threads.emplace_back(multChannel, std::ref(a), std::ref(b), std::ref(c), n,
                         block, std::ref(channel));
  }
  for (auto& thread : threads) {
    thread.join();
  }
}

void deleteMatrix(int** matrix) {
  delete[] matrix[0];
  delete[] matrix;
}

int main() {
  int n = 10;
  int** matr = makeMatrix(n);
  int** matr2 = makeMatrix(n);
  int** matr3 = makeMatrix(n);
  printMatrix(matr, n);
  printMatrix(matr2, n);
  for (int i = 1; i <= n; i++) {
    auto start = std::chrono::steady_clock::now();
    multAll(matr, matr2, matr3, n, i, 10);
    auto end = std::chrono::steady_clock::now();
    int time1 =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start)
            .count();
    std::cout << "blockSize = " << i << ", duration = " << time1
              << ", result of mult: " << '\n';
    printMatrix(matr3, n);
  }
  deleteMatrix(matr);
  deleteMatrix(matr2);
  deleteMatrix(matr3);
}