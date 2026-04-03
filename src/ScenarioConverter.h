#ifndef SCENARIOCONVERTER_H
#define SCENARIOCONVERTER_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>

/**
 * @brief 将 stScenario JSON 转换为 AFSIM 想定 txt 格式
 *
 * 纯静态工具类，不依赖 RA_BASIC 头文件，仅通过 QJsonObject 解析前端传入的 JSON。
 */
class ScenarioConverter
{
public:
    /**
     * @brief 校验 scenario JSON 是否包含必要字段
     * @param scenarioJson  params["scenario"] 对应的 JSON 对象
     * @param errorMsg      校验失败时的错误描述
     * @return true=校验通过
     */
    static bool validateScenario(const QJsonObject& scenarioJson, QString& errorMsg);

    /**
     * @brief 将 stScenario JSON 转换为完整的 AFSIM 想定文本
     * @param scenarioJson  params["scenario"] 对应的 JSON 对象
     * @return AFSIM 想定文本；失败返回空字符串
     */
    static QString convertToAfsimTxt(const QJsonObject& scenarioJson);

private:
    // ---- 映射辅助 ----
    static QString factionToSide(int factionId);
    static QString platTypeToCategory(int typeId);
    static QString platTypeToMover(int typeId, bool isStatic);
    static QString platTypeToIcon(int typeId);
    static QString partTypeToAfsim(int typeId);

    // ---- 格式化辅助 ----
    /// lon/lat(十进制度)/alt(m) → AFSIM position 字符串
    static QString formatPosition(double lon, double lat, double altM);
    /// m/s → knots 字符串
    static QString formatSpeed(double speedMs);

    // ---- 时间计算 ----
    static double calcEndTimeSec(const QString& start, const QString& end);

    // ---- 子块生成 ----
    static QString genPlatformType(const QString& typeName, int typeId, bool isStatic);
    static QString genPlatform(const QString& instanceName,
                               const QString& typeName,
                               const QString& side,
                               const QString& category,
                               double lon, double lat, double altM,
                               double heading, double speedMs,
                               bool isStatic,
                               double endSec,
                               const QJsonObject& payloads);
    static QString genZone(const QString& name, const QJsonObject& area);
    static QString genPayloads(const QJsonObject& payloads);

    // ---- 编队成员绝对位置 ----
    static void calcMemberPosition(double leaderLon, double leaderLat, double leaderAlt,
                                   double leaderHdg,
                                   double relDis, double relHdg, double relAlt,
                                   double& outLon, double& outLat, double& outAlt);
};

#endif // SCENARIOCONVERTER_H
