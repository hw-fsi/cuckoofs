# CuckooFS Cluster Test Setup Guide

Usage:

0. Install ansible with apt.

```
apt update && apt install -y ansible sshpass
```

1. Create your user on all nodes. You can do this manually or by ansible with your own playbook.


```
useradd -m -s /bin/bash cuckoo
passwd #yourpasswd
```

2. Set up SSH key-based authentication for passwordless login under user cuckoo

3. Prepare working directory

```
mkdir -p ~/code/ansible
cd ~/code/ansible
wget https://github.com/hw-fsi/cuckoofs/blob/main/deploy/ansible/inventory
wget https://github.com/hw-fsi/cuckoofs/blob/main/deploy/ansible/cuckootest.yml
```

4. Create .ansible.cfg at your home `~/.ansible.cfg`. The content can be like:

```
[defaults]
inventory = /home/$USER/code/ansible/inventory
log_path = /home/$USER/code/ansible/ansible.log
```

5. Set value in inventory according to the annotation.

6. Install dependencies.

```
ansible-playbook cuckootest.yml --tags install-deps
```

7. Clone code and Build.

```
ansible-playbook cuckootest.yml --tags build
```

8. Start.

```
ansible-playbook cuckootest.yml --tags start
```

9. Stop.

```
ansible-playbook cuckootest.yml --tags stop
```

