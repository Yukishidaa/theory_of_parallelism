## Сборка
С использованием double
```bash
cmake -S . -B build -DUSE_FLOAT=OFF
cmake --build build
./build/main
```
Вывод
```bash
4.89582e-11
```

С использованием float
```bash
cmake -S . -B build -DUSE_FLOAT=ON
cmake --build build
./build/main
```
Вывод
```bash
0.094364
```