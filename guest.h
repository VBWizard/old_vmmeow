void SetupGuestPages();
void HandleRealModeSWInterrupt();
void HandleInterrutpReturn();
void GuestInt13Handler();
void UpdateGuestForSoftwareInterrupt(BYTE vector);
void HandleGuestIO();
void GuestExceptionHandler(WORD ErrorCode);
void HandleGuestPagingException(DWORD Addr);
void HandleCR3Read();
void HandleCR0Write();
void HandleCR3Write();
void HandleCR0Read();

