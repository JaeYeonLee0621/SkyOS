# VFS (Virtual Fle System)

- File system 을 추상화하는 역할

# FileSystem

## FAT (File Allocation Table)

- Window OS 에서 사용하는 FAT System
- 여러 개의 Sector 를 모은 cluster 란 단위로 File 관리
- Sector 의 size 는 일반적으로 512B 이며 Cluster 는 이 값의 배수 크기인 2KB~32KB 단위로 설정

<img width="500" alt="Screen Shot 2023-05-02 at 12 00 14 AM" src="https://user-images.githubusercontent.com/32635539/235472721-142001c2-749e-4dfb-a25e-99544f421f3b.png">

- Storage 를 FAT System 으로 format 하면
- 같은 Directory structure 가 생성
- Directory 는 entry array 이며 entry 는 파일 이름, 생성 시각, 파일이 시작되는 cluster index number를 가짐
- 크기가 한정돼서 파일 이름은 확장자까지 포함해서 11자 밖에 표현할 수 없음
- FAT32 는 더 긴 이름으로 가능

![image](https://user-images.githubusercontent.com/32635539/235472989-e8c2a015-966d-43fa-b132-0183a530377c.png)

- 한 파일의 크기는 최대 4GB 가 한계라 FAT System 에 블루레이 영화를 담기에는 무리가 있음
- FAT entry size = 4byte

<br/>
<br/>

ex) skyos32.exe 란 파일의 데이터를 읽는다고 가정

1. directory entry list 에서 같은 이름을 가진 entry 를 찾음
2. entry 를 찾은 후 cluster index 값을 읽음
3. 화살표가 이어진 대로 계속해서 값을 읽다가 END marking 이 되어 있으면 끝이라는 것

<br/>
<br/>

- FAT System 은 심플하고 직관적이지만, 보안에 취약하며 filesystem 이 손상됬을 때 복구하기 어려움 


> 이런 단점 보완 NFS
