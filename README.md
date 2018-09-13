# 游戏介绍
playerbattle(玩家对战)，是首款完全基于EOS合约实现的经典21点游戏。每局游戏由两名玩家参与，轮流押注，每次出牌(押注)均可获得合约随机分配的1-13(代表扑克牌的A-K)分别记作1-13点。３轮过后（每名玩家均押注3次），玩家总点数接近21点者获胜。胜者除了可以收回自己所有的押注外还可以获得败者押注量的90%（另外10%作为合约的维护及开发成本）。如果两名玩家距离21点的点数相同，则为平局，平局情况下双方将退回各自所有的押注,游戏创建者的游戏押金一并退回


# 玩家角色  
  每局游戏的两名玩家分为两种角色——创建者(creater)、普通玩家(player)。
- 创建者(creater):创建者在开始游戏前需要支付一定的押金(大于等于１ＥＯＳ)作为游戏押金，该押金在正常情况下是不属于creater的押注量，在游戏结束后退回，但是在游戏步数为１时，creater放弃游戏，该押金将不能退回。（此举是为了防止有玩家恶意创建游戏，或者游戏开始后立即结束游戏，浪费合约资源）。每轮中creater均是后手出牌，具有一定的后手优势。优劣互抵。
- 普通玩家(player):普通玩家可以选择参与任何一个状态为０的游戏，玩家需要先出牌，但不需要预先有任何抵押。
　
# 名词解释
 -  游戏步数:　游戏初始状态下步数为0，当有player参与游戏，并成功押注后步数变为１，此后由creater和player轮流出牌，每次出牌后均会使步数+1，当步数到6时（也就是creater最后一次出牌后)，游戏自动结束，合约将计算双方总点数，确定最后胜者并自动分发奖励。
 - 游戏状态: 每局游戏均有3个状态，分别是　０:　游戏刚创建，等待player参与。１:游戏正在进行，双方正在轮流出牌。２:游戏已结束。
 - 游戏押金: 游戏押金由creater在创建游戏时支付，该押金不计入creater的出牌押注量，在游戏正常结束后自动退回。但是需要注意的是:当步数为１是，如果creater放弃游戏(出牌超时或者是主动放弃),该押金将作为奖励90%给player，10％作为合约的开发维护成本。此举是为了防止玩家恶意创建游戏。或者游戏开始后恶意放弃游戏。但是由于creater是属于后手出牌的玩家，具有后手优势，优劣大体相抵。
 - 关闭游戏: creater创建游戏后，在有player成功参与前可以进行关闭游戏操作。关闭游戏后，游戏押金将原数退回。
 - 放弃游戏: 当前出牌玩家可以进行放弃游戏操作，放弃游戏操作将自动判定执行该操作的玩家输，对方将获胜并得到奖励，游戏提前结束。好的一点是执行该操作时，不需要进行更多押注，当玩家根据双方玩家得分觉得获胜希望渺茫时可进行此操作，减少损失。
 - 结束游戏: 如果当前游戏玩家出牌超时(超过15分钟),将视为该玩家放弃游戏。另一个玩家可进行[结束游戏]操作。执行该操作后超时玩家将自动被判定为输，另一玩家获胜，并得到相应奖励。游戏结束。此操作适合于对方玩家长时间不出牌，您将可以通过此操作结束该局游戏。
 - 清理游戏: 当某一局游戏开始后，游戏双方出牌方超时后出牌方不进行[放弃游戏]操作，另一方也不进行[结束游戏]操作，造成游戏长时间不能结束而长期占用合约资源，影响其他玩家游戏体验。超过2个小时，第三方玩家可以对该游戏进行[清理游戏]操作。该第三方玩家将获得双方投注的总和以及creater的游戏押金的90%作为清理奖励。剩余10％作为合约成本。此举是为了防止玩家恶意攻击合约，创建大量游戏占用合约资源。恶意攻击的行为将使攻击者血本无归。同时清理此类游戏的玩家奖获得大额奖励。正常玩家请在游戏过程中及时结束游戏，避免超时。

# 游戏规则

- 游戏押金: 游戏押金由creater支付。需要大于等于1.0000 EOS
- 押注规则: player首次押注量最小不能小于creater的游戏押金，最大不大于游戏押金的3倍，之后游戏双方每次的押注量，最小不能小于对方的最新一次的押注量，最大不能大于对方的最新一次押注量的３倍。如creater游戏押金是1.0000 EOS,player首次出牌的押注量最小为1.000 EOS 最大为3.0000 EOS.  假如player押注量是2.0000 EOS,那么接下来creater出牌的押注量最小为2.0000 EOS 最大为6.0000 EOS，以此类推直到结束。
- 超时规则: 游戏双方每次均有15分钟的时间进行出牌。超时后将不能再进行出牌，当前出牌玩家可以进行[放弃游戏]操作主动认输，或者对方进行[结束游戏]操作，总之超时即为输，请各位玩家在游戏过程中一定要注意是否轮到自己出牌。避免超时。
- 清理游戏规则: 当游戏正常开始后，游戏参与双方出牌方出牌超时后，出牌方不进行[放弃游戏]操作，另一方也不进行[结束游戏]操作。超过两个小时，第三方玩家将可以对该游戏进行清理，清理成功后该第三方玩家将获得游戏双方投注总和及游戏押金的90％作为奖励。因此如果游戏中有一方超时，请参与双方尽快进行[放弃游戏]或者[结束游戏]操作。

# 游戏操作

由于本游戏完全基于EOS合约实现，没有中心化的后台服务，所以玩家有多种方式参与游戏。下面介绍两种。

- 使用ejc.cash提供的网站服务。该网站仅仅是提供了一种玩家参与的方式，没有任何后台计算逻辑，玩家直接与BP进行通信。

玩家首先使用scatter进行登陆。登陆成功后，在[当前游戏列表]栏中将显示[Create New Game] 按钮，通过点击该按钮，玩家可以创建游戏。或者点击游戏列表中的任何一个游戏。将在[游戏信息]列表中显示该游戏的详细信息。网页自动判断当前玩家可进行的操作。并在[游戏信息]栏中显示可进行的操作。
页面下方显示该局游戏参与双方的得分、押注等情况，便于游戏参与方判断游戏形式。
右侧显示所选游戏参与双方的游戏统计信息，胜、败、平场次统计以及在该游戏中的总投入和总收入。（总收入-总投入即为玩家参与本游戏所得的全部收益)

- 使用cleso命令参与游戏

玩家可以直接使用cleos命令进行调用合约相关方法进行相关游戏操作。相关命令如下
```

cleos push action eosio.token transfer '["{useraccount}","playerbattle","1.0000 EOS","create"]' -p eosclubt1@active

```
创建游戏，并支付1.0000 EOS作为游戏押金。


```
cleos get table playerbattle playerbattle currentgame;
```

列出当前正在进行的所有游戏信息，玩家可以选择其中状态为0的游戏进行参与。


```
cleos push action eosio.token transfer '[{accountname},"playerbattle","2.0000 EOS",{gameid}]' -p {accountname}@active

```

为指定的{gameid}的游戏进行押注（出牌)


```
cleos push action playerbattle closegame '[{accountname},{gameid}]' -p {accountname}@active 
```
{accountname}　关闭指定的{gameid}游戏。

```

cleos push action playerbattle claim '["{accountname}","{gameid}"]' -p {accountname}@active

```
结束游戏

```
cleos push action playerbattle giveup '["{accountname}","{gameid}"]' -p {accountname}@active

```
放弃游戏

```
cleos push action playerbattle cleargame '["{accountname}","{gameid}"]' -p {accountname}@active

```
清理游戏


# 游戏公平性

本游戏核心为玩家押注后决定玩家点数生成算法，该算法结合了多种与玩家无关的参数作为算法的输入，每次出牌的点数均为独立生成，之间无相关性。合约提供roll方法，玩家可以通过调用该方法生成随机数，以检验点数生成的公平性。如使用cleos命令
```
mcleos push action playerbattle roll '[]' -p {accountname}@active
```
注意cleos命令是否能打印出点数还与所选用的BP节点有关系。如果没有正常打印出点数，玩家可以连接不同的BP节点进行测试。
玩家还可以使用nodejs等进行批量测试，开发组测试模拟10000次出牌，各点数分布如下(从左到右分别是出现1-13的次数))
```
[764,747,757,785,763,746,757,806,820,746,738,812,759]
```
各点数出现的频次，基本均匀，因为所有玩家使用的同一个随机算法，所以各凭运气，而运气对所有玩家是公平的。
测试核心代码如下
```
var count=[0,0,0,0,0,0,0,0,0,0,0,0,0,0];
testroll(10000)
function testroll(n){
    if(n==0){
        return;
    }

    eos.transaction(
    {
        actions:[
            {
                account: 'playerbattle',
                name: 'roll',
                authorization:[
                    {
                        actor: 'eosjoycenter',
                        permission: 'active'
                    }
                ],
                data: {
                   
                }   
            }
          ]},
        (err,res)=>{
            if(!err){
                //console.log(res.processed.action_traces[0].console)
                var random = res.processed.action_traces[0].console;

                count[random]++;
                testroll(n-1);
                console.log(JSON.stringify(count))

            }else{
                console.log(err)
                testroll(n);
            }
        }
    )
}

```

由于该点数生成算法是使用与玩家无关的随机变量作为输入参数，因此没有玩家可以控制生成的点数。且由于合约中内建玩家游戏数据统计信息。玩家具有很大的自主权去选择自已的对手。

因此玩家获胜的关键点就在于自己的游戏策略。选择合适的进取机会（提高压注量）或者适时的明哲保身才是取得长久利益的关键。
同时为了尽最大限度地保护广大玩家利益，保护合约随机算法不被攻击，团队选择不将该随机点数生成算法开源（后期可以应广大玩家要求，选择开源与否）

# EJC空投

参与玩家将按EOS:EJC=5:1比例获得空投的EJC现阶段，空投总量总计为1000万

# Qusetions

- Q:游戏过程中，如何才能快速定位到自己创建或者参与的游戏?

  Ａ：玩家可以在当前游戏列表中的右侧的输入框中输入自己的用户名称，列表将自动过滤到您参与的游戏。