/*
 * ergo720                Copyright (c) 2023
 */

#include "iop.hpp"
#include "hdd\fatx.hpp"
#include "cdrom\xiso.hpp"
#include "..\rtl\rtl.hpp"


const UCHAR IopValidFsInformationQueries[] = {
  0,
  sizeof(FILE_FS_VOLUME_INFORMATION),    // 1 FileFsVolumeInformation
  0,                                     // 2 FileFsLabelInformation
  sizeof(FILE_FS_SIZE_INFORMATION),      // 3 FileFsSizeInformation
  sizeof(FILE_FS_DEVICE_INFORMATION),    // 4 FileFsDeviceInformation
  sizeof(FILE_FS_ATTRIBUTE_INFORMATION), // 5 FileFsAttributeInformation
  sizeof(FILE_FS_CONTROL_INFORMATION),   // 6 FileFsControlInformation
  sizeof(FILE_FS_FULL_SIZE_INFORMATION), // 7 FileFsFullSizeInformation
  sizeof(FILE_FS_OBJECTID_INFORMATION),  // 8 FileFsObjectIdInformation
  0xff                                   // 9 FileFsMaximumInformation
};

const ULONG IopQueryFsOperationAccess[] = {
   0,
   0,              // 1 FileFsVolumeInformation [any access to file or volume]
   0,              // 2 FileFsLabelInformation [query is invalid]
   0,              // 3 FileFsSizeInformation [any access to file or volume]
   0,              // 4 FileFsDeviceInformation [any access to file or volume]
   0,              // 5 FileFsAttributeInformation [any access to file or vol]
   FILE_READ_DATA, // 6 FileFsControlInformation [vol read access]
   0,              // 7 FileFsFullSizeInformation [any access to file or volume]
   0,              // 8 FileFsObjectIdInformation [any access to file or volume]
   0xffffffff      // 9 FileFsMaximumInformation
};

NTSTATUS IopMountDevice(PDEVICE_OBJECT DeviceObject)
{
	// Thread safety: acquire a device-specific lock
	KeWaitForSingleObject(&DeviceObject->DeviceLock, Executive, KernelMode, FALSE, nullptr);

	NTSTATUS Status = STATUS_SUCCESS;
	if (DeviceObject->MountedOrSelfDevice == nullptr) {
		if (DeviceObject->Flags & DO_RAW_MOUNT_ONLY) {
			RIP_API_MSG("Mounting raw devices is not supported");
		}
		else {
			switch (DeviceObject->DeviceType)
			{
			case FILE_DEVICE_CD_ROM:
				// FIXME: this could also be a regular CD/DVD with UDF filesystem instead of XISO
				Status = XisoCreateVolume(DeviceObject);
				break;

			case FILE_DEVICE_DISK:
				Status = FatxCreateVolume(DeviceObject);
				break;

			case FILE_DEVICE_MEMORY_UNIT:
				RIP_API_MSG("Mounting MUs is not supported");

			case FILE_DEVICE_MEDIA_BOARD:
				RIP_API_MSG("Mounting the media board is not supported");

			default:
				Status = STATUS_UNRECOGNIZED_VOLUME;
			}
		}
	}

	KeSetEvent(&DeviceObject->DeviceLock, 0, FALSE);

	return Status;
}

VOID IopDereferenceDeviceObject(PDEVICE_OBJECT DeviceObject)
{
	KIRQL OldIrql = IoLock();

	--DeviceObject->ReferenceCount;

	if ((DeviceObject->ReferenceCount == 0) && (DeviceObject->DeletePending)) {
		IoUnlock(OldIrql);
		if (DeviceObject->DriverObject->DriverDeleteDevice) {
			DeviceObject->DriverObject->DriverDeleteDevice(DeviceObject);
		}
		else {
			ObfDereferenceObject(DeviceObject);
		}
	}
	else {
		IoUnlock(OldIrql);
	}
}

VOID IopQueueThreadIrp(PIRP Irp)
{
	KIRQL OldIrql = KfRaiseIrql(APC_LEVEL);
	InsertHeadList(&Irp->Tail.Overlay.Thread->IrpList, &Irp->ThreadListEntry);
	KfLowerIrql(OldIrql);
}

VOID IopDequeueThreadIrp(PIRP Irp)
{
	RemoveEntryList(&Irp->ThreadListEntry);
	InitializeListHead(&Irp->ThreadListEntry);
}

VOID ZeroIrpStackLocation(PIO_STACK_LOCATION IrpStackPointer)
{
	IrpStackPointer->MinorFunction = 0;
	IrpStackPointer->Flags = 0;
	IrpStackPointer->Control = 0;
	IrpStackPointer->Parameters.Others.Argument1 = 0;
	IrpStackPointer->Parameters.Others.Argument2 = 0;
	IrpStackPointer->Parameters.Others.Argument3 = 0;
	IrpStackPointer->Parameters.Others.Argument4 = 0;
	IrpStackPointer->FileObject = nullptr;
}

VOID IoMarkIrpPending(PIRP Irp)
{
	IoGetCurrentIrpStackLocation(Irp)->Control |= SL_PENDING_RETURNED;
}

PIO_STACK_LOCATION IoGetCurrentIrpStackLocation(PIRP Irp)
{
	return Irp->Tail.Overlay.CurrentStackLocation;
}

PIO_STACK_LOCATION IoGetNextIrpStackLocation(PIRP Irp)
{
	return Irp->Tail.Overlay.CurrentStackLocation - 1;
}

VOID XBOXAPI IopCloseFile(PVOID Object, ULONG SystemHandleCount)
{
	if (SystemHandleCount == 1) {
		PFILE_OBJECT FileObject = (PFILE_OBJECT)Object;
		FileObject->Flags |= FO_HANDLE_CREATED;

		if (FileObject->Flags & FO_SYNCHRONOUS_IO) {
			IopAcquireSynchronousFileLock(FileObject);
		}

		PDEVICE_OBJECT DeviceObject = FileObject->DeviceObject;
		PIRP Irp = IoAllocateIrpNoFail(DeviceObject->StackSize);
		Irp->Tail.Overlay.OriginalFileObject = FileObject;
		Irp->Tail.Overlay.Thread = (PETHREAD)KeGetCurrentThread();
		Irp->UserIosb = &Irp->IoStatus;
		Irp->Flags = IRP_SYNCHRONOUS_API | IRP_CLOSE_OPERATION;

		PIO_STACK_LOCATION IrpStackPointer = IoGetNextIrpStackLocation(Irp);
		IrpStackPointer->MajorFunction = IRP_MJ_CLEANUP;
		IrpStackPointer->FileObject = FileObject;

		IopQueueThreadIrp(Irp);

		IofCallDriver(DeviceObject, Irp);

		KIRQL OldIrql = KfRaiseIrql(APC_LEVEL);
		IopDequeueThreadIrp(Irp);
		KfLowerIrql(OldIrql);

		IoFreeIrp(Irp);

		if (FileObject->Flags & FO_SYNCHRONOUS_IO) {
			IopReleaseSynchronousFileLock(FileObject);
		}
	}
}

VOID XBOXAPI IopDeleteFile(PVOID Object)
{
	RIP_UNIMPLEMENTED();
}

NTSTATUS XBOXAPI IopParseFile(PVOID ParseObject, POBJECT_TYPE ObjectType, ULONG Attributes, POBJECT_STRING CompleteName, POBJECT_STRING RemainingName,
	PVOID Context, PVOID *Object)
{
	RIP_UNIMPLEMENTED();
}

VOID XBOXAPI IopCompleteRequest(PKAPC Apc, PKNORMAL_ROUTINE *NormalRoutine, PVOID *NormalContext, PVOID *SystemArgument1, PVOID *SystemArgument2)
{
	RIP_UNIMPLEMENTED();
}

VOID IopDropIrp(PIRP Irp, PFILE_OBJECT FileObject)
{
	if (Irp->UserEvent && FileObject && !(Irp->Flags & IRP_SYNCHRONOUS_API)) {
		ObfDereferenceObject(Irp->UserEvent);
	}

	if (FileObject && !(Irp->Flags & IRP_CREATE_OPERATION)) {
		ObfDereferenceObject(FileObject);
	}

	IoFreeIrp(Irp);
}

NTSTATUS IopSynchronousService(PDEVICE_OBJECT DeviceObject, PIRP Irp, PFILE_OBJECT FileObject, BOOLEAN DeferredIoCompletion, BOOLEAN SynchronousIo)
{
	IopQueueThreadIrp(Irp);

	NTSTATUS Status = IofCallDriver(DeviceObject, Irp);

	if (DeferredIoCompletion) {
		if (Status != STATUS_PENDING) {
			PKNORMAL_ROUTINE NormalRoutine;
			PVOID NormalContext;

			KIRQL OldIrql = KfRaiseIrql(APC_LEVEL);
			IopCompleteRequest(&Irp->Tail.Apc, &NormalRoutine, &NormalContext, (PVOID *)&FileObject, &NormalContext);
			KfLowerIrql(OldIrql);
		}
	}

	if (SynchronousIo) {
		if (Status == STATUS_PENDING) {
			KeWaitForSingleObject(&FileObject->Event, Executive, KernelMode, FALSE, nullptr);
			Status = FileObject->FinalStatus;
		}

		IopReleaseSynchronousFileLock(FileObject);
	}

	return Status;
}

VOID IopAcquireSynchronousFileLock(PFILE_OBJECT FileObject)
{
	if (InterlockedIncrement(&FileObject->LockCount)) {
		KeWaitForSingleObject(&FileObject->Lock, Executive, KernelMode, FALSE, nullptr);
	}
}

VOID IopReleaseSynchronousFileLock(PFILE_OBJECT FileObject)
{
	if (InterlockedDecrement(&FileObject->LockCount) >= 0) {
		KeSetEvent(&FileObject->Lock, 0, FALSE);
	}
}

NTSTATUS IopCleanupFailedIrpAllocation(PFILE_OBJECT FileObject, PKEVENT EventObject)
{
	if (EventObject) {
		ObfDereferenceObject(EventObject);
	}

	if (FileObject->Flags & FO_SYNCHRONOUS_IO) {
		IopReleaseSynchronousFileLock(FileObject);
	}

	ObfDereferenceObject(FileObject);

	return STATUS_INSUFFICIENT_RESOURCES;
}
