# 📦 C Inventory Management System

A simple and efficient command-line Inventory Management System written in C.

This project demonstrates structured programming, file handling, and modular design in C.  
It is designed for learning purposes and small-scale inventory tracking.

---

## 🚀 Features

- Add new inventory items
- View all items
- Search for items
- Update item details
- Delete items
- Persistent storage using file handling
- Clean and modular C code

---

## 🛠️ Technologies Used

- C Programming Language
- GCC Compiler
- Standard C Library
- File Handling (`fopen`, `fread`, `fwrite`)

---

## 📂 Project Structure

Inventory-management-system/
│
├── inventory.c # Main program
├── inventory.txt # Storage file (generated at runtime)
├── README.md
├── LICENSE
└── .gitignore


---

## ⚙️ Installation & Compilation

### 1️⃣ Clone the repository

git clone https://github.com/Chege-N/Inventory-management-system.git
cd Inventory-management-system


### 2️⃣ Compile the program

gcc inventory.c -o inventory


### 3️⃣ Run the program

Linux / macOS:
./inventory


Windows:
inventory.exe


---

## 🧠 How It Works

The system uses:

- `struct` to represent inventory items
- File handling for data persistence
- A menu-driven CLI interface
- Functions for modular program structure

Example structure:

```c
struct Item {
    int id;
    char name[50];
    int quantity;
    float price;
};
📸 Sample Output
1. Add Item
2. View Items
3. Search Item
4. Update Item
5. Delete Item
6. Exit

Enter your choice:
