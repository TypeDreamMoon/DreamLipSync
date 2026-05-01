# DreamLipSync

`DreamLipSync` 是一个基于 Morph Target 的口型同步插件，支持在 Sequencer 中播放，也支持运行时通过组件调用播放。

## 核心资产

创建 `DreamLipSyncClip` DataAsset，用同一份资产同时服务 Sequencer 和 Runtime。

默认 Viseme 映射适配当前卡通模型的 MorphTarget 命名：

| Viseme | MorphTarget |
| --- | --- |
| Neutral | Fcl_MTH_Neutral |
| A | Fcl_MTH_A |
| I | Fcl_MTH_I |
| U | Fcl_MTH_U |
| E | Fcl_MTH_E |
| O | Fcl_MTH_O |
| Close | Fcl_MTH_Close |

`DreamLipSyncClip` 支持两种数据模式：

| 模式 | 用途 |
| --- | --- |
| Viseme Keys | 按时间写 `A/I/U/E/O/Close/Neutral`，插件自动插值并映射到 MorphTarget。 |
| Morph Frames | 直接按时间写多个 MorphTarget 权重，适合外部工具导出的曲线。 |

常用时间参数：

| 字段 | 说明 |
| --- | --- |
| Playback Blend Time | `Viseme Keys` 播放时嘴型之间的混合时间。值越小切换越利落，值越大越柔和。 |
| Generation Blend Time | MFA / Rhubarb 生成 `MorphFrames` 时使用的嘴型过渡时间；NVIDIA ACE 会直接烘焙连续曲线。 |

### Viseme Recipes

`Viseme Recipes` 用来在识别出的基础嘴型上叠加其他嘴型或直接叠加 MorphTarget。它适合处理卡通模型里“单一 O 太死”“E/I 太硬”“A 需要带一点下唇”等问题。

例如让 `O` 同时带一点 `U` 和 `I`：

```text
Source Viseme = O
bIncludeBaseViseme = true
BaseScale = 1.0
VisemeMixes:
  U = 0.20
  I = 0.10
ExtraMorphWeights:
  Fcl_MTH_Up = 0.05
```

执行顺序是：

```text
MFA/Rhubarb 识别出 O
-> Viseme Recipes 展开成 O + U + I + Extra Morph
-> Viseme Mappings 映射到 Fcl_MTH_*
-> 写入 MorphFrames
```

注意：`Viseme Recipes` 会影响 `Generate From MFA`、`Generate From Rhubarb` 和 `Viseme Keys` 播放；如果 Clip 已经生成过 `MorphFrames`，修改 Recipe 后需要重新生成。

## Sequencer 使用

1. 在 Level Sequence 中绑定角色 Actor 或 SkeletalMeshComponent。
2. 在绑定对象上添加 `Dream Lip Sync Track`。
3. 点击 Track 右侧 `LipSync` 按钮添加 Section。
4. 在 Section Details 中设置 `Clip`。
5. Audio Track 仍然使用 UE 原生音频轨道，和 LipSync Section 对齐即可。

Section 的时间控制：

| 字段 | 说明 |
| --- | --- |
| Auto Size To Clip Duration | 设置 Clip 后自动把 Section 长度改成 Clip 的有效时长。 |
| Lock Section Length To Clip Duration | 防止把 Section 拉得超过 Clip 时长。 |
| Clamp Evaluation To Clip Duration | 即使 Section 被拉长，超过 Clip 时长后也会停止评估并重置嘴型。 |
| Stretch Clip To Section | 把 Clip 时间拉伸到整个 Section 长度，适合刻意放慢/加长口型。 |

右键 Lip Sync Section 可以执行：

```text
Set Length To Clip Duration
```

## 离线生成口型

插件支持三种非商业优先的离线流程：

| 方案 | 适合场景 | 说明 |
| --- | --- | --- |
| NVIDIA ACE | 需要更自然的连续曲线 | 直接调用本地 `NV_ACE_Reference` / Audio2Face-3D provider，把 ACE 输出烘焙到 `MorphFrames`。 |
| MFA | 中文/多语言台词，已知文本 | 用 Montreal Forced Aligner 生成音素级 TextGrid，再映射到 `A/I/U/E/O/Close/Neutral`。 |
| Rhubarb | 快速占位、英文或简单音频 | 直接从 `.wav` / `.ogg` 生成 mouth cues，质量通常低于 MFA/ACE。 |

所有生成流程都会把 `DataMode` 切到 `MorphFrames`，并覆盖当前 `MorphFrames`。

### Clip 需要设置

| 字段 | 说明 |
| --- | --- |
| Sound | 对应音频。必须是从原始音频导入的 `SoundWave`，或者设置 `Source Audio File Override`。 |
| Locale | 例如 `zh-CN`、`en-US`、`ja-JP`。 |
| Source Audio File Override | 当 SoundWave 找不到原始导入文件时，手动指定音频文件。NVIDIA ACE 离线生成当前要求这里最终解析到 `.wav`。 |
| Dialogue Text | MFA 必填；Rhubarb 可选。填入与音频一致的台词文本。 |

### 生成参数覆盖

Project Settings 里的生成参数是全局默认值；如果某条对白需要单独调，可以在 `DreamLipSyncClip` 资产里打开对应生成器的 `Override Project Settings`：

| 资产字段 | 作用 |
| --- | --- |
| Rhubarb Generation Settings | 单独覆盖 `Recognizer` 和 `Extended Shapes`。 |
| MFA Generation Settings | 单独覆盖 MFA root、dictionary、acoustic model、并行数、config、speaker、G2P、clean/overwrite/quiet/fine tune 等参数。 |
| NVIDIA ACE Generation Settings | 单独覆盖 ACE provider、burst mode 和超时时间。 |
| Frame Generation Settings | 单独覆盖生成后的曲线细节，例如时间偏移、最短嘴型、强度、静音回 Neutral、固定帧率烘焙和平滑。 |

这样可以做到同一个项目里中文、英文、不同角色或不同录音质量使用不同 MFA / ACE 参数，不需要频繁改全局设置。

`Frame Generation Settings` 是 MFA / Rhubarb / ACE 共用的生成后处理参数：

| 参数 | 说明 |
| --- | --- |
| Time Offset Seconds | 整体提前或延后生成口型。负值用于让嘴型更早预备。 |
| Min Cue Duration | 把过短的音素/嘴型撑到最小时长，减少闪烁。 |
| Cue End Padding | 延长每个识别到的嘴型尾部。 |
| Weight Scale / Max Weight | 调整生成曲线强度和上限。 |
| Neutral Viseme | 静音回落时使用的嘴型，默认 `Neutral`。 |
| Insert Neutral Frames In Gaps | 长静音间隙自动插入 Neutral，避免嘴型一直挂住。 |
| Add Neutral Frame At Start / End | 生成曲线首尾补 Neutral。 |
| Bake Fixed Frame Rate | 按固定帧率烘焙连续曲线，方便后续平滑和人工编辑。 |
| Smoothing Passes / Strength | 对生成曲线做时间平滑。 |

### NVIDIA ACE 离线生成

这个流程不再走 Audio2Face JSON。插件直接调用 NVIDIA ACE Unreal Plugin 的 Audio2Face-3D provider，接收 ACE 输出的 ARKit 风格 blendshape 曲线，然后按 `SourceMorphMappings` 写入当前 `DreamLipSyncClip.MorphFrames`。

这里的“离线”指编辑器内一次性烘焙到资产，不代表不需要 ACE provider。DreamLipSync 对 `NV_ACE_Reference` 是可选依赖：没有 ACE 插件时，DreamLipSync 仍可编译和运行，只是不会提供 `Generate From NVIDIA ACE` 生成功能。

当前 `NV_ACE_Reference` 包默认 `Default` 会解析到 `RemoteA2F`；如果没有配置 A2F-3D Server URL，DreamLipSync 会优先自动 fallback 到可用的 `LocalA2F-*` provider。

1. 确认项目启用了 `NV_ACE_Reference` 插件，并且它能正常连接本地或远端 Audio2Face-3D provider。
2. 打开：

   ```text
   Project Settings -> DreamPlugin -> Dream Lip Sync
   ```

3. 设置：

   | 设置项 | 说明 |
   | --- | --- |
| Enable Ace Generation | 关闭后 DreamLipSync 会隐藏 ACE 生成入口，但 Sequencer / Runtime / MFA / Rhubarb 仍然正常使用。 |
| Ace Provider Name | 下拉会列出当前 ACE 已注册 provider。推荐使用本地 provider，例如 `LocalA2F-James`。`Default` 会先走 ACE 默认 provider，远端 URL 为空时会 fallback 到可用的 `LocalA2F-*`。 |
| Ace Force Burst Mode | 离线生成推荐开启，让 ACE 尽快生成完整曲线。 |
| Ace Generation Timeout Seconds | 等待 ACE 返回完整动画的超时时间。 |
| Use Emotion Parameters | 启用后把 DreamLipSync 的 Emotion Parameters 传给 ACE。关闭时保持原先的无情绪参数调用。 |
| Emotion Parameters | 对应 ACE 的 `Audio2Face-3D Emotion Parameters`，包含检测情绪强度、对比度、平滑、最大情绪数量，以及 Joy / Anger / Sadness 等手动情绪覆盖。 |

   远端 provider 还需要在下面的位置设置服务地址：

   ```text
   Project Settings -> Plugins -> NVIDIA ACE -> Default A2F-3D Server Config
   ```

   本机服务通常类似：

   ```text
   http://127.0.0.1:52000
   ```

4. 在 `DreamLipSyncClip.SourceMorphMappings` 中把 ACE 曲线映射到当前模型 MorphTarget。
5. 在 Content Browser 中右键 `DreamLipSyncClip`：

   ```text
   Generate From NVIDIA ACE
   ```

默认 `SourceMorphMappings` 已包含一组常见 Audio2Face/ARKit 风格名称：

| Source | MorphTarget |
| --- | --- |
| jawOpen | Fcl_MTH_A |
| mouthClose | Fcl_MTH_Close |
| mouthFunnel | Fcl_MTH_O |
| mouthPucker | Fcl_MTH_U |
| mouthStretchLeft / mouthStretchRight | Fcl_MTH_E |
| mouthUpperUpLeft / mouthUpperUpRight | Fcl_MTH_Up |
| mouthLowerDownLeft / mouthLowerDownRight | Fcl_MTH_Down |

### 推荐：MFA 中文/多语言强制对齐

MFA 不直接“猜嘴型”，它会根据音频和台词文本对齐到音素时间轴。中文、多国语言项目通常比 Rhubarb 的纯音频/粗略口型更稳定。

1. 安装 Montreal Forced Aligner，并安装对应语言模型。例如中文可使用 `mandarin_china_mfa` dictionary + `mandarin_mfa` acoustic model。

   Windows 推荐创建 Python 3.10 环境，中文分词依赖在该版本上更稳：

   ```powershell
   conda create -n aligner310 -c conda-forge python=3.10 montreal-forced-aligner
   conda activate aligner310
   python -m pip install spacy-pkuseg dragonmapper hanziconv
   mfa model download acoustic mandarin_mfa
   mfa model download dictionary mandarin_china_mfa
   where mfa
   ```

   如果使用中文模型但缺少分词依赖，MFA 会报：

   ```text
   Please install Chinese tokenization support via `pip install spacy-pkuseg dragonmapper hanziconv`
   ```
2. 打开：

   ```text
   Project Settings -> DreamPlugin -> Dream Lip Sync
   ```

3. 设置：

   | 设置项 | 说明 |
   | --- | --- |
   | Mfa Command | 默认 `mfa`。如果环境变量找不到，就填 `aligner310\Scripts\mfa.exe` 的完整路径。 |
   | Mfa Root Directory | MFA 模型和缓存目录。C 盘空间紧张时建议放到其他盘，例如 `I:/MFA/Data`。 |
   | Mfa Dictionary | 中文普通话推荐 `mandarin_china_mfa`，也可以填自定义词典路径。 |
   | Mfa Acoustic Model | 默认 `mandarin_mfa`，也可以填自定义模型路径。 |
   | Mfa Num Jobs | 并行任务数。 |
   | Mfa Extra Arguments | 追加到 `mfa align` 的原始高级参数。常用参数建议优先用下面的明确字段。 |
   | Blend Time | 音素切换过渡时间，默认 `0.04` 秒。 |

   常用 MFA 高级参数也已暴露到 Project Settings 的 `MFA|Advanced`，并且可以在单个 `DreamLipSyncClip.MFA Generation Settings` 中覆盖：

   | 设置项 | 说明 |
   | --- | --- |
   | Mfa Config Path | 使用 MFA YAML 配置文件。 |
   | Mfa Speaker Characters | 按文件名前 N 个字符识别说话人，或填 `prosodylab`。 |
   | Mfa G2P Model Path | 给 OOV 词使用的 G2P 模型。 |
   | Clean / Overwrite / Quiet / Verbose | 对应 MFA 的清理、覆盖、日志输出选项。 |
   | Use Multiprocessing / Use Threading | 控制 MFA 并行方式。 |
   | Single Speaker | 单说话人模式，适合单条对白素材。 |
   | No Tokenization | 禁用预训练分词；中文普通话通常保持关闭。 |
   | TextGrid Cleanup | 输出 TextGrid 后处理。 |
   | Fine Tune | 额外边界微调，耗时更长。 |

4. 在 Content Browser 中右键 `DreamLipSyncClip`：

   ```text
   Generate From MFA
   Import MFA TextGrid
   ```

也可以在 Sequencer 中右键 `Dream Lip Sync Track`：

```text
Generate Clips From MFA
```

### 配置 Rhubarb

1. 下载 Rhubarb Lip Sync。
2. 打开：

   ```text
   Project Settings -> DreamPlugin -> Dream Lip Sync
   ```

3. 设置：

   | 设置项 | 说明 |
   | --- | --- |
   | Rhubarb Executable Path | `rhubarb.exe` 路径。 |
   | Rhubarb Recognizer | `Auto` 会按 Clip 的 `Locale` 选择。`en-*` 使用 PocketSphinx，其他语言使用 Phonetic。 |
   | Extended Shapes | 默认 `GHX`。 |
   | Blend Time | cue 切换过渡时间，默认 `0.04` 秒。 |

### 生成方式

在 Content Browser 中右键 `DreamLipSyncClip`：

```text
Generate From Rhubarb
Import Rhubarb JSON
```

也可以在 Sequencer 中右键 `Dream Lip Sync Track`：

```text
Generate Clips From Rhubarb
```

## Runtime 使用

给角色添加 `DreamLipSyncComponent`：

```text
DreamLipSyncComponent
-> TargetMesh: 角色脸部 SkeletalMeshComponent
-> DefaultClip: DreamLipSyncClip
```

蓝图调用：

```text
Play Lip Sync Clip(Clip, bPlayAudio = true)
Stop Lip Sync()
Set Lip Sync Playback Time()
```

如果 `DreamLipSyncClip.Sound` 已设置，Runtime 播放时可以同步播放音频。

## 多语言建议

每种语言使用独立 Clip 和音频：

```text
Dialogue_001_zh_CN: Audio + DreamLipSyncClip
Dialogue_001_en_US: Audio + DreamLipSyncClip
Dialogue_001_ja_JP: Audio + DreamLipSyncClip
```

所有语言都可以映射到同一套 MorphTarget。
