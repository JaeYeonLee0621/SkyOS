
# [Virtual Address Space]
- Process 가 참조할 수 있는 주소의 범위로 하나의 `Process 당 하나의 Virtual Address Space 가 주어짐`
- Process 는 Thread 를 담는 Container
- 실제 코드 실행은 Thread 에서 수행되는데, `하나의 Process 에는 복수개의 Thread 가 존재` 하며
- `이런 Thread 는 Virtual Address Space 를 공유` 

<br/>

### 32bit window OS 에서는 4GB Virtual Address Space 이 주어짐
- 이 중 2GB 까지는 응용 프로그램이 사용할 수 있는 주소 공간
- 나머지 2GB 는 Kernel 의 가상 주소 공간

2GB 로 직접적인 접근을 불가능
System API 를 호출해서 간접적으로 Kernel Code 수행


<br/>
<br/>


# [Paging]
- 메모리 공간을 Page 단위로 나눠서 사용하는 방법
- 보호 모드 전환에 의해 32bit address 주소 접근이 가능하더라도 실제 Physical Memory 가 4GB 이하라면 4GB 주소 전체에 접근하는 것은 불가능
- Paging 기술을 활용해서는 가능

- `Segmentation` : 프로그램의 가상 주소를 Linear Address 로 변환하는 과정
- `Paging` : Linear Address 를 얻었으면, 이 값을 이용해서 실제 물리 메모리를 기술하는 Page 를 찾음

<br/>

**32bit 에서 보통 Page 의 크기 4KB**

- 32 bit address 에서는 4GB Memory 를 사용할 수 있고
- 4KB Page 는 4GB 에서 `2^20 개의 Page` 를 가질 수 있음
- `2^20 개` 의 Page 를 저장할 수 있는 Table 이 존재해야 함
- 즉 Page 를 저장하는 공간의 Address 가 20bit 여야함


<br/>
<br/>



# 과정

![VM](https://user-images.githubusercontent.com/32635539/235419731-c3a7f73d-858f-4f93-8a9e-c9edd2cf5b45.png)

<br/>

## 1. Segmentation 을 거쳐 Linear Address 를 얻어냄

- `Linear Address` = `Page directory entry index address (10bit)` + `Page table entry index address (10bit)` + `offset (12bit)`

<br/>

## 2. Page Directory

- 10bit address = 1024 개
- Page Table 이 있는 address 를 모아놓은 곳
- `PDBR (Page Directory Base Register)` 가 Page Directory 에 대한 Base Pointer 유지
- 한 개의 값들을 Page Directory Entry 라고 부름

**Page Directory Entry**
- 4byte 의 크기를 가짐
- 여러 가지 Flag 와 Page Table 의 Base Address에 대한 정보를 가지고 있음

<br/>

## 3. Page Table

- 10bit address = 1024 개 
- Page 의 Physical Address 가 저장됨
- 한 개의 값들을 Page Table Entry 라고 부름

**Page Table Entry**
- 4byte 의 크기를 가짐
- 그런데 20bit 만 사용 나머지는 offset 으로 이용
- 왜냐하면 Page 개수가 2^20 개 (4GB memory / 4KB page size)
- 따라서 20bit 의 주소 필요

<br/>

## 4. Page Physical Address
- 20bit Address = 해당 Page 가 있는 시작 주소
- Linear Address 의 Offset 12bit 를 이용하여 계산
- 해당 부분이 실제 Page 가 있는 Physical Address

<hr/>

즉 

- 1024 * 1024 * 4KB = 4GB

> 특정 Process의 Memory 침범으로 인해 다른 Process 가 망가지는 것을 막을 수 있음

<br/>

- 그런데 4GB 에 접근하기 위해 모든 Page Table 이 메모리에 생성된다면 굉장한 메모리 낭비 (Page Table 의 크기는 4KB = 1024 * 4byte)
- 이 값은 Process 하나 당 필요로 하는 값이며 Process 가 많아지면 메모리 사용량은 더 증가
+) 만약 모든 Page Directory 에 Page Table 을 생성한다면 `2^10 * 4KB = 4MB`

> 일반적으로는 4GB 전 공간에 접근한다고 하더라도 실제 접근할 수 있는 주소는 한정돼 있기 때문에 모든 Page Table 을 생성하지는 않음
