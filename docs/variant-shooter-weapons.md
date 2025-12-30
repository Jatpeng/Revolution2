## Variant_Shooter/Weapons 模块文档

本模块实现射击玩法的核心武器系统，包含武器本体、弹丸、拾取物与持有者接口，路径：`Source/Revolution2/Variant_Shooter/Weapons/`。

### 组成与职责
- `AShooterWeapon`（抽象）：武器基类，管理第一/第三人称网格、开火/弹药/后坐力/连发与动画；通过 `IShooterWeaponHolder` 与持有者交互。
- `AShooterProjectile`（抽象）：弹丸基类，处理运动、碰撞、伤害、物理冲量、（可选）爆炸与延迟销毁；提供命中蓝图事件。
- `AShooterPickup`（抽象）：武器拾取物，从数据表行加载外观与授予的 `WeaponClass`，支持重生与蓝图驱动的重生动画。
- `IShooterWeaponHolder`（接口）：武器持有者必须实现的接口（角色/AI），用于挂接网格、播放开火动画、添加后坐力、更新 HUD、计算瞄准点、授予武器等。

---

### AShooterWeapon
关键属性（可在详情面板编辑）：
- 视角网格：`FirstPersonMesh`、`ThirdPersonMesh`
- 弹药：`MagazineSize`（默认 10）、`CurrentBullets`（运行时）
- 弹丸：`ProjectileClass`（`AShooterProjectile` 子类）
- 动画：`FiringMontage`、`FirstPersonAnimInstanceClass`、`ThirdPersonAnimInstanceClass`
- 瞄准：`AimVariance`（散布，度）、`FiringRecoil`（后坐力）、`MuzzleSocketName`、`MuzzleOffset`
- 连发：`bFullAuto`、`RefireRate`（秒）、内部 `RefireTimer`/`bIsFiring`
- 感知：`ShotLoudness`、`ShotNoiseRange`、`ShotNoiseTag`

主要方法：
- 生命周期：`BeginPlay` / `EndPlay`、`OnOwnerDestroyed`
- 切换：`ActivateWeapon()` / `DeactivateWeapon()`
- 开火控制：`StartFiring()` / `StopFiring()`、`Fire()`、`FireCooldownExpired()`
- 发射：`FireProjectile(TargetLocation)`、`CalculateProjectileSpawnTransform(TargetLocation)`
- 访问器：`GetFirstPersonMesh()`、`GetThirdPersonMesh()`、`GetFirstPersonAnimInstanceClass()`、`GetThirdPersonAnimInstanceClass()`、`GetMagazineSize()`、`GetBulletCount()`

持有者交互（通过 `IShooterWeaponHolder`）：
- `AttachWeaponMeshes(Weapon)`：将武器网格挂接到角色
- `PlayFiringMontage(Montage)`、`AddWeaponRecoil(Recoil)`
- `UpdateWeaponHUD(CurrentAmmo, MagazineSize)`
- `GetWeaponTargetLocation()`：获取瞄准点
- `AddWeaponClass(WeaponClass)`、`OnWeaponActivated/Deactivated(Weapon)`、`OnSemiWeaponRefire()`

使用建议：
- 在具体武器子类中设置 `ProjectileClass`、动画与瞄准参数；确保 `MuzzleSocketName` 与武器网格插槽一致。
- 半自动武器依赖 `RefireRate` 节流；全自动依赖 `RefireTimer` 重复触发。

---

### AShooterProjectile
组件：
- `CollisionComponent`（`USphereComponent`）
- `ProjectileMovement`（`UProjectileMovementComponent`）

关键属性：
- 噪声（AI 感知）：`NoiseLoudness`、`NoiseRange`、`NoiseTag`
- 命中效果：`PhysicsForce`、`HitDamage`、`HitDamageType`、`bDamageOwner`
- 爆炸：`bExplodeOnHit`、`ExplosionRadius`
- 销毁：`DeferredDestructionTime`、`DestructionTimer`

主要方法：
- 生命周期：`BeginPlay` / `EndPlay`
- 碰撞：`NotifyHit(...)`
- 处理：`ProcessHit(...)`、`ExplosionCheck(ExplosionCenter)`
- 蓝图：`BP_OnProjectileHit(Hit)`（命中回调，可做特效/音效/震屏等）

使用建议：
- 通过 `ProjectileMovement` 配置速度、重力、反弹；在命中回调中触发视觉/音频反馈。
- 开启爆炸时注意 `ExplosionRadius` 与伤害归属（团队伤害规则）。

---

### AShooterPickup
数据行结构 `FWeaponTableRow`：
- `StaticMesh`：拾取物显示的网格（软引用）
- `WeaponToSpawn`：拾取后授予的武器类（`TSubclassOf<AShooterWeapon>`）

关键属性：
- 组件：`SphereCollision`、`Mesh`
- 数据：`WeaponType`（`FDataTableRowHandle`），用于设置 `WeaponClass` 与显示网格
- 重生：`RespawnTime`、`RespawnTimer`

主要方法：
- `OnConstruction(Transform)`：从数据表行加载外观/武器类
- 生命周期：`BeginPlay` / `EndPlay`
- 触发：`OnOverlap(...)` 拾取逻辑（对持有者授予 `WeaponClass`）
- 重生：`RespawnPickup()`、`BP_OnRespawn()`（蓝图动画后调用）→ `FinishRespawn()` 启用交互

使用建议：
- 在数据表配置多种拾取类型，便于关卡布置；通过蓝图实现重生动画与提示。

---

### IShooterWeaponHolder（接口）
必须由角色或控制武器的对象实现：
- `AttachWeaponMeshes(Weapon)`、`PlayFiringMontage(Montage)`、`AddWeaponRecoil(Recoil)`
- `UpdateWeaponHUD(CurrentAmmo, MagazineSize)`：与 UI 同步
- `GetWeaponTargetLocation()`：命中/弹道目标点
- `AddWeaponClass(WeaponClass)`、`OnWeaponActivated/Deactivated(Weapon)`、`OnSemiWeaponRefire()`

实现要点：
- 第一/第三人称骨骼网格应分别有对应的插槽与动画实例类。
- UI（如弹药数、准星）应订阅或在回调中刷新。

---

### 集成步骤（示例）
1. 在角色类实现 `IShooterWeaponHolder`，完成网格挂接、动画播放、HUD 更新与瞄准计算。
2. 创建具体武器蓝图类（继承 `AShooterWeapon`），设置：网格、`ProjectileClass`、动画、`MuzzleSocketName`、弹药与连发参数。
3. 创建弹丸蓝图类（继承 `AShooterProjectile`），配置碰撞、运动、伤害与命中特效，在 `BP_OnProjectileHit` 中实现效果。
4. 建立拾取物数据表并放置 `AShooterPickup` 派生蓝图于关卡，选择 `WeaponType` 行，实现 `BP_OnRespawn` 动画。

---

### 调试与排错
- 看不到弹丸：检查 `ProjectileClass`、`MuzzleSocketName`、碰撞通道、`ProjectileMovement` 初速。
- 无法连发：检查 `bFullAuto` 与 `RefireRate`；确认 `StartFiring/StopFiring` 的调用时机。
- 弹药/HUD 不更新：确保接口 `UpdateWeaponHUD` 被调用且 UI 绑定正确。
- 自伤：关闭 `bDamageOwner` 或在命中处理中过滤拥有者。
