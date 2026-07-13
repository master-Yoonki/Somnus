# Somnus 세션 핸드오프 (2026-07-13)

> Claude Code → 후속 어시스턴트 인수인계 문서.
> 이 파일 하나로 맥락을 파악하고 "진행 중인 작업"부터 이어가면 된다.
> 이 파일은 커밋하지 말 것 (로컬 작업 메모).

---

## 0. 절대 규칙 (작업 방식)

1. **멘토 모드**: 유저는 DigiPen RTIS 졸업, C++ 강하고 UE 특유 패턴을 학습 중.
   - **게임플레이 코어 코드(GAS/AI/캐릭터/리플리케이션/애니메이션 C++)는 유저가 직접 타이핑한다.**
     어시스턴트는 설계·구조·정확한 삽입 위치·UE 패턴 설명·diff 리뷰만 제공.
   - 어시스턴트가 직접 작성해도 되는 것: 툴링/스크립트(예: `Scripts/`의 UE Python),
     빌드 진단, 기계적 상수 수정(유저가 명시적으로 부탁했을 때).
   - 유저가 "그냥 해줘"라고 해도 코어 코드면 체크리스트로 되돌려주는 게 기본값.
2. **포트폴리오 품질**: 모든 코드는 미국 UE 개발직 지원용 포트폴리오 소재 (2026 말 완성 목표).
   UE 컨벤션 엄수, 디버그 잔재 금지, 학습 설명은 대화로 하고 코드 주석은 non-obvious WHY만.
3. 대화는 한국어. 서버 권위 멀티플레이어가 기본 전제 (인벤토리·GAS·AI 전부).

## 1. 프로젝트 한 줄 요약

UE 5.7 / C++ / GAS 기반 2~4인 코옵 좀비 서바이벌 (쇼핑몰 배경).
마스터 플랜: `zombie_survival_project_plan.md` (프로젝트 루트, 로드맵의 source of truth).
현재 브랜치: `feat/grid-inventory`. 그리드 인벤토리 Phase A는 이전에 완료됨.
요즘은 **좀비 AI + 애니메이션 트랙** 진행 중.

## 2. 오늘 완료된 것 (커밋 완료, 빌드 그린, PIE 검증됨)

- `b963513` — Zombie_Anims 팩 임포트+리타겟, 플레이어 애니 `Content/Animation/Player/`로 재구조화
- `4af4895` — 좀비 로코모션 시스템 + 래그돌 사망:
  - `FZombieStateConfig` + `TMap<EZombieState, FZombieStateConfig> StateConfigs`
    (`SomnusZombieAIController.h/.cpp`) — `SetState()`에서 `ApplyStateConfig()`로
    상태별 `MaxWalkSpeed` 적용. 서버에서만 세팅 → velocity 복제로 클라 애니 자동 동기화.
  - `USomnusZombieAnimInstance` (`Source/Somnus/Animation/`) — `GroundSpeed`/`bIsMoving`/`bIsDead`
  - `ABP_Zombie` (`Content/Animation/Zombie/`) — Idle 상태(Random Player 노드, 랜덤 아이들)
    ↔ Locomotion 상태(`BS_ZombieLocomotion` 2D 블렌드스페이스), 전환 조건 `bIsMoving`
  - `BS_ZombieLocomotion` — X=Direction(-180~180), Y=Speed(0~700).
    **Direction 핀은 상수 0으로 고정** (strafe는 의도적으로 나중, 아래 §5 참조)
  - 좀비 사망: `Die()` → `MulticastZombieDeath()` RPC → 래그돌 (플레이어 패턴 미러)
- 풋슬라이딩 거의 해결됨 (속도-애니 정렬 방식, 아래 데이터 참조)

### 측정된 애니메이션 고유 속도 (pelvis 수평이동/길이, `Scripts/measure_zombie_speeds.py`)

| 클립 | cm/s | 쓰임 |
|---|---|---|
| Zombie_Walk_04_Forward | 22 | BS 전진 열 |
| Zombie_Walk_01_Forward (+방향 링) | 38 | Unaware 속도, BS 방향 링 |
| Zombie_Walk_Fast01_Forward | 127 | Aware 속도, BS 전진 열 |
| Zombie_Run_01_Forward (+방향 링) | 320 | Hunt_Predict 속도, BS 방향 링 |
| Zombie_Run_02_Forward | 540 | BS 전진 열 |
| Zombie_Sprint_01/02_Forward | 697 | Hunt_Certain 속도, BS 전진 열 |

### 상태 → 속도/의미 매핑 (StateConfigs 생성자 기본값)

Unaware 38 (셔플 배회) / Aware 127 (빠른 걸음 경계) / Hunt_Predict 320 (런, 시야 잃고 예측 추격)
/ Hunt_Certain 697 (스프린트, 타겟 보임) / Attack 0.
→ 플레이어가 시야를 끊으면 좀비가 감속하는 emergent 스텔스 요소. 의도된 디자인.

## 3. ★ 진행 중인 작업 (여기서부터 이어갈 것)

**좀비 사망 코드 정리 — 유저가 타이핑하는 중이거나 시작 직전.**
파일: `Source/Somnus/Character/Zombie/SomnusZombieCharacter.cpp` (+.h)
참고 원본: 플레이어 `SomnusCharacter.cpp:390` `Die()` / `:413` `MulticastDeath_Implementation()`

리뷰에서 지적된 남은 이슈 (동작은 하나 실버그 계열):
1. **이중 사망 가드 없음** — `OnHealthChanged`가 체력≤0에서 반복 호출되면 `Die()` 재실행됨
2. **`State_Dead` 태그를 안 붙임** — `SomnusZombieAnimInstance.bIsDead`가 영원히 false,
   BT/어빌리티 가드도 무근거
3. **`CancelAllAbilities()` 안 함** — 죽는 순간 어택 어빌리티 생존
4. **authority 로직이 멀티캐스트 안에 있음** — `DetachFromControllerPendingDestroy()`,
   `SetLifeSpan(5.f)`는 서버 전용이므로 `Die()`로 이동해야 함

목표 구조:
```
Die()  // 서버
  1. State_Dead 태그 있으면 return (가드)
  2. ASC->CancelAllAbilities()
  3. ASC->AddLooseGameplayTag(SomnusTags::State_Dead)
  4. MulticastZombieDeath()          // 썽크 호출 (★ _Implementation 직접 호출 금지)
  5. DetachFromControllerPendingDestroy()
  6. SetLifeSpan(5.f)

MulticastZombieDeath_Implementation()  // 전 머신, 비주얼만
  - 캡슐 NoCollision
  - GetCharacterMovement()->SetMovementMode(MOVE_None)   // 추가 권장
  - 메시 Detach + Ragdoll 프로필 + SimulatePhysics + WakeAllRigidBodies (현재 코드 유지)
```
include: `GameFramework/CharacterMovementComponent.h` 필요할 수 있음. 태그는
`Core/SomnusGameplayTags.h`의 `SomnusTags::State_Dead`.

완료되면: diff 리뷰 → 빌드 → PIE (죽은 좀비 또 때려도 재사망 없음 확인).

## 4. 다음 작업 (오늘 세션 목표, 순서대로)

**A. 좀비 공격 몽타주 연결** — `SomnusGA_ZombieMeleeAttack.cpp`의 TODO(딜레이 시뮬레이션)를 실제 몽타주로.
1. `ABP_Zombie` AnimGraph에 **DefaultSlot 노드 추가** (SM 출력 → Slot → Output Pose). 아직 없음!
2. `Content/Zombie_Anims/RetargetedAnims/`의 `Zombie_Attack_01`~ 중 골라 AnimMontage 생성
   (주의: RetargetedAnims에 Attack 시퀀스가 아직 없으면 리타겟 필요할 수 있음 — 확인할 것)
3. 어빌리티에서 기존 태스크 재사용:
   `USomnusAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(OwningAbility,
   TaskInstanceName, Montage, EventTags, Rate, StartSection, bStopWhenAbilityEnds, RootMotionScale)`
   (`Source/Somnus/AbilitySystem/Tasks/`) — 플레이어 근접공격 어빌리티가 사용 예시.
4. 히트 판정: 우선 몽타주 완료/이벤트 기반, 시간 남으면 AnimNotify로 히트 타이밍.
   플레이어는 `USomnusAnimNotifyState_MeleeTrace` 사용 중 — 좀비도 재사용 가능.
5. BP `BP_SomnusGA_ZombieMeleeAttack`에 몽타주 지정 후 PIE.

**B. (시간 남으면) 사망 diff/공격까지 커밋** — 커밋 메시지 스타일은 `git log` 참조
(`feat:`/`fix:`/`chore:` prefix, Co-Authored-By 라인).

## 5. 의도적으로 미룬 것 (건드리지 말고 알고만 있을 것)

- **Strafe/Direction**: BS는 2D로 만들어놨지만 ABP에서 Direction 핀 상수 0.
  이유: orient-to-movement 상태에서 Direction을 물리면 회전 랙 동안 스트레이프가
  잘못 섞이는 아티팩트. 나중에 "근접 교전 패스"에서 facing 정책 전환
  (Attack 근처에서 타겟 지향 + `CalculateDirection`)과 함께 켤 것.
- 전진 열에 Walk_01@38, Run_01@320 앵커 추가 (Unaware/Predict 상태의 미세 슬라이드 제거)
  — PIE에서 거슬리면 그때. 현재 유저 판단: 충분히 괜찮음.
- `RotationRate` 좀비답게 낮추기 (기본 360°/s → 90~150 추천) — 회전 뻣뻣함 개선 레버.
- Stride Warping / Orientation Warping (UE Animation Warping 플러그인) — 폴리시 단계.
- `Zombie_Turn_*` turn-in-place 몽타주 — 폴리시 단계.
- `RetargetedAnims` 밖의 원본/중복(`…1` 접미사) 정리 — 나중에 일괄.

## 6. 툴링 / 환경 노트

- **빌드**: PowerShell에서
  `& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" SomnusEditor Win64 Development -Project="C:\Dev\Unreal Projects\Somnus\Somnus.uproject" -WaitMutex`
- **UE Python 스크립트** (`Scripts/`): 애셋 읽기/수정 자동화.
  `measure_zombie_speeds.py`(애니 고유속도 측정), `fix_blendspace_speeds.py`(BS 샘플 Y 정렬),
  `inspect_blendspace.py`, `check_bs_axis.py`.
  실행: `UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<스크립트>" -unattended -nop4 -nosplash -stdout -FullStdOutLogOutput`
  **애셋을 쓰는 스크립트는 반드시 에디터 닫고 실행** (클로버 방지). 실행 전 `tasklist`로 UnrealEditor 확인.
  UE 5.7 API 주의: `AnimSequence.extract_root_motion` 없음. `AnimationLibrary.get_bone_pose_for_frame(seq, bone, frame, True)` 사용.
- **빌드가 `SomnusModuleRules.dll` App Control 차단(0x800711C7)으로 실패하면**:
  `Intermediate/Build/BuildRules/` 안의 dll/pdb/json 삭제 후 재빌드 (오늘 발생, 해결됨).
- 유저 오타 주의: 채팅에 한글 자모 무의미 문자열이 오면 키 입력 사고일 수 있음 —
  동시에 소스 파일에도 문자가 새어들어갔던 사례 있음 (`}1`, 파일 끝 `2`로 컴파일 에러).

## 7. 로드맵상 다음 마일스톤 (오늘 이후)

- 좀비 히트리액트 (`Zombie_HitReact_*` 리타겟됨), 사망 폴리시
- 인벤토리 Phase B (loose loot pickup, `APickupActor` — 아직 미구현)
- **Phase B/순서3 (아이템 사용→GE, 무기 장착→ASC) 들어가기 전에 GAS 아키텍처
  워크스루 세션을 하기로 약속되어 있음** — 유저에게 상기시킬 것.
