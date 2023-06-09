![image](https://user-images.githubusercontent.com/32635539/235434124-9bc30981-dd69-4509-b9c9-518afacafb39.png)

# Heap

- 동적인 메모리 할당 시 memory resource 를 효율적으로 사용하기 위해 반드시 필요한 기능

<br/>

- `Stack` : Compile time 에 따라 크기가 결정
- `Heap` : default 1MB 근데 늘여달라면 더 늘여줌

## Stack
- 접근이 빠름
- Stack pointer 만 변경하면 되므로 memory deallocation 이 없음
- OS 에 의해 효율적으로 관리되기 때문에 fragmentation 현상은 발생하지 않음 
- stack 에 생성되는 local 변수는 call function 에서만 접근 가능
- stack size 는 일반적으로 limitation 에 있음 (depend on OS)
- allocation size 는 재조정될 수 없음

## heap
- 접근이 느림
- 명시적인 memory deallocation 필요
- 공간의 효율적 사용 보장 X & allocation, deallocation 의 반복으로 인해 fragmentation 이 발생 가능
- heap 을 통해 생성한 변수는 전역적으로 접근 가능
- heap 의 크기에 제한은 없음
- realloc 함수를 통해 할당된 크기를 변경할 수 있음


<br/>
<br/>

**[heap 에 접근이 느린 이유]**

- memory allocation algorithm 과정이 복잡
- memory 재사용을 위해 해당 memory 를 회수해야하는데, heap data structure 에 회수될 memory 를 적절하게 넣어야 함
- stack 은 자신만의 stack 이 이지만 heap 은 모든 thread 에 공유 > `동기화 처리 속도 저하 문제`
- Thread race condition 에 따른 heap 의 memory allocation 속도 저하를 피하기 위해 `TLS (Thread Local Storage)` 를 사용하기도 함

**[TLS (Thread Local Storage)]**

- Thread 자체만의 local storage
- shared resource 접근 시 race condition 을 일으키는 것을 막기 위해

<br/>
<br/>

# Kernel Heap Algorithm

- JamesM 의 Kernel Development Tutorial 에서 소개한 heap algorithm 활용
- `block`, `hole` 이라는 두가지 concept 활용

<br/>

- `block` : 현재 사용 중에 있는 user data 를 포함하는 영역
- `hole` : 사용하지 않는 영역
- 즉 처음 사용될 때는 heap 은 커다란 hole

<br/>

- block 에 접근하기 위해서는 block 으로의 pointer 를 담은 descriptor table 이 필요
- `descriptor table` : heap index table or index table 이라고 부름

<br/>

![image](https://user-images.githubusercontent.com/32635539/235435386-c3d19601-a9ec-42d7-9d5c-4df46c314355.png)

+) 아래 쪽 상자의 앞부분 = header / 뒷부분 = footer

<br/>

- 이론적으로 32768개의 block 생성 가능 
- `block` : 자신을 기술하는 header, footer data 포함
- `header` : block 에 대한 정보
- `footer` : header 를 가리키는 pointer data

<br/>

**[header]**

- `is_hole` : is allocated memory space
- `size` : header structure size
- `magic` : checksum, 값이 수정되었다면, heap curruption 이 일어났다는 것

<br/>
<br/>

# Allocation

- 기본적으로 Memory Allocation 시 error check & memory fragmentation 을 최소화 하는 방향으로 새로운 hole 생성

![image](https://user-images.githubusercontent.com/32635539/235461990-33e76a1f-9541-4cec-9fdc-da4a32bd0bf2.png)

1. 요청된 memory size 를 수용할 수 있는 가장 작은 hole 을 찾기 위해 index table 검사
- index table 은 memory size 가 작은 순으로 정렬돼 있기 때문에 요구된 크기에 맞는 hole 을 찾을 때까지 index table 크기를 선형 검색

2. 할당하려는 memory size 가 매우 커서 이를 만족시킬 수 있는 hole 이 없다면, heap 의 size 를 늘임

3. hole 을 생성한 뒤 header & footer 에 정확한 값을 기입하고 index table 에 이 block 을 가리키는 index pointer 를 더함

<br/>

![image](https://user-images.githubusercontent.com/32635539/235462005-54b34726-d2e2-49a9-9746-4a18ac3652b5.png)

- 만약 block space 가 너무 크다면, block 을 두 개로 쪼갬

<br/>
<br/>

# Deallocation

![image](https://user-images.githubusercontent.com/32635539/235462310-fd8204fd-815d-4ebe-978b-9614c92e7e19.png)

- block 회수 시 hole을 병합

- 회수된 block 양쪽 모두가 사용중이라면 병합할 수 없음




