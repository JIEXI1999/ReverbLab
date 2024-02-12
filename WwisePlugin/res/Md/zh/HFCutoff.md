##Damping Parameter

高频混响信号衰减的截止频率

阻尼单元（Damping）是施加在扩散器序列（详见revalg.h DiffusionStep模板）上的IIR搁架滤波器，用于模拟反射声碰撞室内多孔材质物面后高频能量的流失。滤波器Q值为0.5。

**Note**: 取值接近15000时自动停用

单位: 赫兹 <br/>