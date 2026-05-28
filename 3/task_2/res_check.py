import math
import os

def check_client_file(filename, expected_func):
    if not os.path.exists(filename):
        print(f"[-] Файл {filename} не найден. Проверь, запущен ли C++ сервер.")
        return

    print(f"[*] Проверка {filename}...")
    errors = 0
    total = 0

    with open(filename, 'r', encoding='utf-8') as f:
        for line in f:
            if not line.strip():
                continue
            
            try:
                parts = line.split('|')
                arg_val = float(parts[1].split(':')[1].strip())
                res_val = float(parts[2].split(':')[1].strip())
                
                reference = expected_func(arg_val)
                
                if math.isclose(res_val, reference, rel_tol=1e-7):
                    total += 1
                else:
                    print(f"    [!] Ошибка в строке: {line.strip()}")
                    print(f"        Ожидалось: {reference}, Получено: {res_val}")
                    errors += 1
            except Exception as e:
                print(f"    [!] Ошибка парсинга строки: {e}")

    if errors == 0:
        print(f"[+] Успешно! Проверено задач: {total}. Ошибок нет.")
    else:
        print(f"[-] Проверка провалена. Ошибок: {errors} из {total + errors}.")

if __name__ == "__main__":
    check_client_file("client_1.txt", math.sin)
    
    check_client_file("client_2.txt", math.sqrt)
    
    check_client_file("client_3.txt", lambda x: math.pow(x, 2))