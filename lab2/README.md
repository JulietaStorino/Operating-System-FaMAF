# Operating Systems Lab 2 2023 - Group 17 (FaMAF UNC)

## Group Members:

- Victor Alejandro Díaz Jáuregui (victor.diaz.jauregui@mi.unc.edu.ar)
- Julieta Paola Storino (julieta_storino@mi.unc.edu.ar)
- Rocio Guadalupe Gomez (rocio.guadalupe.gomez@mi.unc.edu.ar)
- Esmeralda Choque (esmeralda.choque@mi.unc.edu.ar)


# How to run the code?

## Installation

1. Clone repository: `git clone git@bitbucket.org:sistop-famaf/so23lab2g17.git`

2. Install qemu (on ubuntu): `sudo apt-get install qemu-system-riscv64 gcc-riscv64-linux-gnu`
 
## Compilation and execution

### Compile and run XV6

Inside the "so23lab2g17" directory, run the following command:

```sh
make qemu
```
### Run pingpong

 After completing the previous step, to use the pingpong command, type: 

```sh
pingpong N
```
where N is a natural number.

## Compilation and execution of tests

Inside the "so23lab2g17" directory, run the following command:

```sh
make grade
```

The above command allows the tests to be executed to verify the correct operation of the implemented command (pingpong). To see that they were carried out effectively, the OK message of the 5 tests is displayed on the screen, as well as a "Score: 5/5" at the end.

# Comments

- The original **README** file was renamed **README_xv6**.

- Instead of the **original README** file, another **README** file with the **extension .md** was created, which is this and serves as a basic execution guide.

- The **Makefile** file was modified in **lines 139 and 140** (referring to the previous commit) since the **.md** extension was added to the **README** mentions, since without them the **Makefile** could not be used.