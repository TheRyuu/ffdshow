[code]
function IsProcessorFeaturePresent(Feature: Integer): Boolean;
external 'IsProcessorFeaturePresent@kernel32.dll stdcall';

function Is_SSE_Supported(): Boolean;
begin
  Result := IsProcessorFeaturePresent(6);
end;

function Is_SSE2_Supported(): Boolean;
begin
  Result := IsProcessorFeaturePresent(10);
end;

type
  TSystemInfo = record
    wProcessorArchitecture: Word;
    wReserved: Word;
    dwPageSize: DWORD;
    lpMinimumApplicationAddress: Integer;
    lpMaximumApplicationAddress: Integer;
    dwActiveProcessorMask: DWORD;
    dwNumberOfProcessors: DWORD;
    dwProcessorType: DWORD;
    dwAllocationGranularity: DWORD;
    wProcessorLevel: Word;
    wProcessorRevision: Word;
  end;

procedure GetSystemInfo(var lpSystemInfo: TSystemInfo); external 'GetSystemInfo@kernel32.dll stdcall';

function GetCPULevel(): Integer;
var
  SysInfo: TSystemInfo;
begin
  GetSystemInfo(SysInfo);

  Result := SysInfo.wProcessorLevel;
end;

function GetNumberOfCores(): Integer;
var
  SysInfo: TSystemInfo;
begin
  GetSystemInfo(SysInfo);

  Result := SysInfo.dwNumberOfProcessors;
  if Result > 8 then begin
    Result := 8;
  end;
  if Result < 1 then begin
    Result := 1;
  end;
end;
