# Somnus — Zombie AI Design v2

> **Created:** 2026-07-07
> **Status:** Design Finalized — Implementation Ready
> **Supersedes:** Somnus - Zombie AI Plan.md (기존 구현 폐기, 클린 재구축)
> **Related Systems:** GAS (Melee Combat), NavMesh, EQS

---

## 핵심 아키텍처 원칙

- **상태 전환은 100% C++ 컨트롤러에서.** BT는 Blackboard 값에 반응만 한다.
- **AI 상태 머신 = 인지(무엇을 아는가), GAS = 행동 실행(공격 등).** 두 축을 섞지 않는다.
- `SetState()`가 유일한 상태 전환 통로. Blackboard의 `ZombieState` 키를 갱신하면, 각 브랜치의 데코레이터(`Observer aborts: Both`)가 실행 중인 태스크를 끊고 즉시 해당 브랜치로 점프한다.

---

## 상태 정의 (4 States)

```cpp
UENUM(BlueprintType)
enum class EZombieState : uint8
{
    Unaware,        // 미인지 — 벡터장 배회
    Aware,          // 열받은 상태 — 예민한 탐색 (누구에게인지는 중요하지 않음)
    HuntCertain,    // 확실한 추적 — 실시간 위치로 추격 + 공격
    HuntPredict     // 예측 추적 — 외삽 위치로 이동, 재발견 시도
};
```

### Unaware
- 벡터장 기반 배회: EQS로 목표점 선정, **forward 방향 가중치 높음**
- 경로가 막히면 자연스럽게 우회 (NavMesh + 가중치 조정)
- Perception 활성 (기본 감도/범위)

### Aware
- **패트롤하지 않음** — 진행 방향의 관성 유지 (갑자기 휙 도는 것 방지)
- 벡터장 재사용하되 **bias 방향 = 자극이 온 방향 / 기억 위치 방향**
- Perception 예민화: 범위 확대 + 반응 속도 향상
- "열받은 상태" — 특정 대상에 묶이지 않음

### HuntCertain
- 정확한 actor location으로 실시간 추적 (보이는 동안은 offset 없음)
- 사거리 내 진입 시 공격 (내부 sub-selector, 별도 상태 아님)
- 시야 상실 후에도 `CertainGraceTime`(~1.5초) 동안 유지 — 모멘텀 표현

### HuntPredict
- 시야 상실이 확정된 후 진입
- **외삽(extrapolation)** 위치로 이동 — 실시간 위치 추적 아님 (월핵 방지)
- 플레이어가 직진하면 잘 따라오고, 급커브 틀면 속음 → 카운터플레이 제공
- 예측 위치 도달 후 미발견 시 Aware로 폴백

---

## 상태 전환 규칙표

모든 전환은 C++ 컨트롤러에서 판단. BT는 관여하지 않는다.

| 현재 | 트리거 | 다음 | 비고 |
|---|---|---|---|
| Unaware | Perception 감지 (원거리/약한 자극) | Aware | 기억 위치 저장 |
| Unaware / Aware | 피격 (거리 무관) | HuntCertain | |
| Unaware / Aware | 근접 노출 (~200cm) | HuntCertain | |
| Unaware / Aware | 시야 누적 시간 초과 (거리에 비례) | HuntCertain | 가까울수록 빨리 전환 |
| HuntCertain | 시야 상실 → GraceTime(1.5s) 경과 | HuntPredict | 외삽 계산 후 진입 |
| HuntPredict | 시야로 재발견 | HuntCertain | |
| HuntPredict | 예측 위치 도달 + 미발견 | Aware | 탐색 중심 = **좀비 현 위치** (기억 위치 아님 — 이미 소모된 정보) |
| Aware | AwareDuration 경과 | Unaware | 메모리 전체 삭제 |
| **모든 상태** | **사거리 내 피격** | **HuntCertain + 타겟 즉시 전환** | Hard override — 스코어링 스킵 |

---

## 타이머 설계 (분리 필수)

| 타이머 | 기본값 | 의미 |
|---|---|---|
| `MemoryTimeout` | 5s | 위치 정보가 stale해지는 시간. 이후엔 기억 위치로 가지 않고 일반 탐색 |
| `AwareDuration` | 15s | "열받음"이 식는 시간. 경과 시 Unaware 복귀 |
| `CertainGraceTime` | 1.5s | 시야 상실 후 Certain 유지 버퍼 (모멘텀) |

> [!warning]
> Memory와 Aware를 같은 값으로 묶으면 "5초 숨으면 완전 리셋"이 되어 긴장감이 사라진다. 반드시 분리.

---

## 클래스 구성

```
ASomnusZombieCharacter        — 폰. ASC + AttributeSet(Health) 소유
ASomnusZombieAIController     — 상태 머신 + Perception + 타겟팅의 주인 (Single Source of Truth)
UBTService_ThreatAssessment   — 0.3초 간격 타겟 스코어링 (5단계에서 부착)
UBTTask_ZombieAttack          — GAS 어빌리티 트리거 후 완료 대기
EQS Query: Q_WanderPoint      — 선호 방향 가중치 벡터장 (Unaware/Aware 공용)
```

### 컨트롤러가 소유하는 데이터

```cpp
// --- 상태 ---
EZombieState CurrentState;
void SetState(EZombieState NewState);   // 유일한 전환 통로. BB 갱신 + 진입/이탈 처리

// --- 타겟 정보 ---
APawn*  CurrentTarget;
FVector LastKnownLocation;      // 마지막 본 위치
FVector LastKnownVelocity;      // 외삽용
float   LastSeenTime;

// --- 타이머 ---
float MemoryTimeout    = 5.f;
float AwareDuration    = 15.f;
float CertainGraceTime = 1.5f;
```

---

## Blackboard 키

| 키 | 타입 | 쓰는 곳 | 읽는 곳 |
|---|---|---|---|
| `ZombieState` | Enum | `SetState()` | 각 브랜치 데코레이터 |
| `TargetActor` | Object | Perception / Threat Service | Certain의 MoveTo |
| `PredictedLocation` | Vector | 외삽 계산 (C++) | Predict의 MoveTo |
| `SearchLocation` | Vector | EQS 결과 | Unaware / Aware의 MoveTo |

---

## Behavior Tree 구조

Selector는 왼쪽 우선 → **우선순위 높은 상태가 위로** (Hunting → Aware → Unaware 순).

```
Root Selector
│  [Service: ThreatAssessment — 5단계에서 부착]
│
├─ Sequence [Decorator: ZombieState == HuntCertain, Observer aborts: Both]
│   └─ Selector
│       ├─ Sequence [사거리 내]
│       │   └─ Attack Task (GAS → Event.Melee.Hit)
│       └─ Move To TargetActor
│
├─ Sequence [Decorator: ZombieState == HuntPredict, Observer aborts: Both]
│   └─ Move To PredictedLocation
│
├─ Sequence [Decorator: ZombieState == Aware, Observer aborts: Both]
│   ├─ RunEQS → SearchLocation (자극 방향 bias)
│   ├─ Move To SearchLocation
│   └─ Wait (짧게)
│
└─ Sequence [Decorator: ZombieState == Unaware, Observer aborts: Both]
    ├─ RunEQS → SearchLocation (forward bias)
    ├─ Move To SearchLocation
    └─ Wait 2-5s
```

### 공격은 상태가 아니다
- 공격 = HuntCertain 내부의 지역 로직 (sub-selector)
- "공격 중" 상태성(이동 불가, 캔슬 규칙 등)은 GAS가 GameplayTag로 관리 (`State.Attacking` 등)
- BT의 Attack Task는 어빌리티를 트리거하고 끝날 때까지 대기만 한다

---

## 외삽 로직 (HuntPredict 진입 시)

```cpp
// SetState(HuntPredict)의 진입 처리에서
float Elapsed = Now - LastSeenTime;
FVector Predicted = LastKnownLocation + LastKnownVelocity * Elapsed;
// NavMesh 위로 프로젝션 후 Blackboard에 쓰기
BB->SetValueAsVector("PredictedLocation", Predicted);
```

- 저장할 것: 시야 상실 시점의 위치 + 속도 (두 개뿐)
- 여러 좀비가 같은 기억을 공유해도 외삽 + 개별 offset으로 자연스럽게 분산됨

---

## Perception 설계

- **항상 활성화** (모든 상태)
- Unaware: 기본 범위/감도
- Aware: 범위 확대 + 반응 속도 향상 (둘 다)
- 소리 시스템은 현 단계 제외 — 구조상 자극(stimulus) 방향만 받으면 되므로 추후 청각 추가가 쉬움

---

## 벡터장 / EQS 설계

- **주기:** 매 3초마다 EQS로 새 목표점 (매 틱 아님 — 성능)
- **스폰 시 타이머에 랜덤 오프셋** — 쿼리가 같은 프레임에 몰리지 않게
- **이동:** Simple Movement (Steering) — NavMesh가 자동 회피
- **선호 방향 파라미터** 하나로 통일:
  - Unaware → 현재 forward 방향
  - Aware → 자극 / 기억 위치 방향
- 막힌 방향은 가중치 하락, 우회 방향 가중치 상승

---

## Multi-Player Threat Assessment (2층 구조)

### 1층 — Hard Override (스코어링 스킵)
- 사거리 내에서 피격 → **즉시 타겟 전환 + HuntCertain**
- 점수 시스템과 분리해 hysteresis와의 충돌 방지, 디버깅 용이

### 2층 — Soft Scoring (BTService, 0.3s 간격)
평가 요소:
1. **거리** — 가까울수록 높음
2. **최근 피해** — 피격 직후 최대, 시간에 따라 decay
3. **피해 크기** — 현재 타겟의 피해량과 비교
4. **시야 포착 시간** — 거리에 비례한 가중치
5. **현재 타겟 hysteresis 보너스** (+0.25) — 잦은 전환 방지

동작:
- 현재 타겟을 쫓으려는 경향성 유지, 시간이 갈수록 그 우선도가 decay → 어그로 플레이 가능
- 새 타겟 점수가 (현재 타겟 + 보너스)를 넘어야 전환

---

## 구현 순서 (Milestones)

각 단계는 테스트 가능한 상태로 완결:

- [x] **1. 뼈대** — Character + Controller + NavMesh, `EZombieState` + `SetState()` + BB, 빈 브랜치 4개(전부 Wait). *테스트: 디버그 키로 강제 전환 → BT 브랜치가 즉시 따라 바뀌는가*
- [x] **2. Unaware** — EQS 벡터장(forward bias) + 배회. *테스트: 혼자 자연스럽게 배회하는가*
- [x] **3. Aware** — Perception 연결, Unaware→Aware 전환, 자극 방향 bias + 감도 상승. *테스트: 멀리서 지나가면 그 방향으로 슬금슬금 오는가*
- [x] **4. Hunting** — Certain 추적 → GraceTime → Predict 외삽 → 실패 시 Aware 폴백. *테스트: 1:1 숨바꼭질이 재밌는가 (핵심 테스트)*
- [ ] **5. 멀티 타겟팅** — Threat Service + Hard override. *1:1이 재밌은 후에만 착수*
- [ ] **6. GAS 공격** — Attack Task, Event.Melee.Hit, Health 차감

---

## 설계 결정 기록 (Why)

| 결정 | 이유 |
|---|---|
| 상태 전환을 C++로 (BT 데코레이터 아님) | 전환 조건이 복잡(피격/거리/시야시간/Threat) — BT로 표현 시 스파게티. 타이머 로직도 C++가 깔끔 |
| Predict = 외삽 (실시간 추적 아님) | 실시간 추적은 사실상 월핵. 외삽은 직진엔 강하고 급커브엔 속아 카운터플레이 제공. 구현도 더 단순 |
| Memory / Aware 타이머 분리 | 묶으면 "5초 숨으면 리셋" — 긴장감 붕괴 |
| 사거리 내 피격 = Hard rule | 점수에 녹이면 hysteresis와 충돌 케이스 발생. 공격 가능한데 지나쳐가는 부자연스러움 원천 차단 |
| Predict 실패 후 탐색 중심 = 좀비 현 위치 | 기억 위치는 이미 지나쳐서 소모된 정보 |
| 공격은 상태 아님 | 상태 머신은 인지 축, 공격은 행동 축. GAS가 행동 상태성 관리 |
| Aware 탐색 = Unaware 벡터장 재사용 | 시스템 통일. bias 방향 파라미터 하나만 추가 |
