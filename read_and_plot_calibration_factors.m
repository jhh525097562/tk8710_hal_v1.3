% 读取校准因子数据并绘制散点图
% 作者: 自动生成
% 日期: 2026-03-27

function read_and_plot_calibration_factors()
    % 文件路径
    filename = 'CaliFactor/CaliFactor.txt';
    
    % 检查文件是否存在
    if ~exist(filename, 'file')
        error('校准因子文件不存在: %s', filename);
    end
    
    % 读取文件内容
    fid = fopen(filename, 'r', 'n', 'UTF-8');
    if fid == -1
        error('无法打开文件: %s', filename);
    end
    
    % 读取所有行
    lines = textscan(fid, '%s', 'Delimiter', '\n');
    lines = lines{1};
    fclose(fid);
    
    % 解析数据
    calibration_data = parse_calibration_data(lines);
    
    % 转换为浮点数并绘制散点图
    plot_calibration_scatter(calibration_data);
    
    % 显示统计信息
    display_statistics(calibration_data);
end

function calibration_data = parse_calibration_data(lines)
    calibration_data = struct();
    calibration_data.timestamps = {};
    calibration_data.i_factors = [];
    calibration_data.q_factors = [];
    
    current_i = zeros(1, 8);  % 8个通道的I路校准因子
    current_q = zeros(1, 8);  % 8个通道的Q路校准因子
    collecting = false;
    channel_count = 0;
    
    for i = 1:length(lines)
        line = lines{i};
        
        % 检查是否是时间戳行
        if startsWith(line, '===') && endsWith(line, '===')
            % 如果之前有数据，先保存
            if collecting && channel_count == 8
                calibration_data.timestamps{end+1} = line;
                calibration_data.i_factors(end+1, :) = current_i;
                calibration_data.q_factors(end+1, :) = current_q;
            end
            
            % 重置状态
            current_i = zeros(1, 8);
            current_q = zeros(1, 8);
            channel_count = 0;
            collecting = true;
            continue;
        end
        
        % 检查是否是校准因子数据行
        if collecting && contains(line, '通道[') && contains(line, 'I_factor=') && contains(line, 'Q_factor=')
            % 解析通道号和校准因子
            % 格式: 通道[X]: I_factor=0xYYYYY, Q_factor=0xZZZZZ
            
            % 提取通道号
            channel_idx = extractBetween(line, '通道[', ']:');
            channel_idx = str2double(channel_idx{1}) + 1;  % MATLAB索引从1开始
            
            % 提取I_factor和Q_factor
            tokens = regexp(line, 'I_factor=0x([0-9A-Fa-f]+), Q_factor=0x([0-9A-Fa-f]+)', 'tokens');
            if ~isempty(tokens)
                i_factor_hex = tokens{1}{1};
                q_factor_hex = tokens{1}{2};
                
                i_factor = hex2dec(i_factor_hex);
                q_factor = hex2dec(q_factor_hex);
            else
                warning('无法解析I/Q因子: %s', line);
                continue;
            end
            
            % 存储数据
            if channel_idx >= 1 && channel_idx <= 8
                current_i(channel_idx) = i_factor;
                current_q(channel_idx) = q_factor;
                channel_count = channel_count + 1;
            end
        end
    end
    
    % 保存最后一组数据
    if collecting && channel_count == 8
        calibration_data.timestamps{end+1} = lines{end-8};  % 时间戳
        calibration_data.i_factors(end+1, :) = current_i;
        calibration_data.q_factors(end+1, :) = current_q;
    end
    
    % 转换18位补码为浮点数
    calibration_data.i_float = convert_18bit_to_float(calibration_data.i_factors);
    calibration_data.q_float = convert_18bit_to_float(calibration_data.q_factors);
end

function float_data = convert_18bit_to_float(hex_data)
    % 将18位有符号数转换为浮点数
    % 参数：
    %   hex_data: 原始18位数据矩阵
    % 返回：
    %   float_data: 转换后的浮点数矩阵
    
    % 18bit有符号数的范围：-2^17 到 2^17-1
    % 小数位15bit，所以需要除以2^15
    SCALE_FACTOR = 2^15;  % 15bit小数位对应的缩放因子
    
    % 处理有符号数
    % 如果数值大于等于2^17，则为负数（补码表示）
    NEGATIVE_THRESHOLD = 2^17;
    
    [num_calibrations, num_channels] = size(hex_data);
    float_data = zeros(num_calibrations, num_channels);
    
    for i = 1:num_calibrations
        for j = 1:num_channels
            value = hex_data(i, j);
            value = bitand(value, 0x3FFFF);  % 确保18位
            Tmp = double(value);
            % 检查是否为负数（补码）
            if Tmp >= NEGATIVE_THRESHOLD
                % 转换为负数
                Tmp = Tmp - 2^18;
            end
            
            % 转换为浮点数（除以2^15）
            float_data(i, j) = Tmp / SCALE_FACTOR;
        end
    end
end

function plot_calibration_scatter(calibration_data)
    % 绘制I-Q散点图
    if isempty(calibration_data.i_float) || isempty(calibration_data.q_float)
        warning('没有可用的校准数据');
        return;
    end
    
    figure('Name', 'I-Q散点图', 'Position', [100, 100, 800, 600]);
    
    % 定义8种有效的颜色和标记
    colors = {'red', 'blue', 'green', 'magenta', 'cyan', 'black', 'yellow', [0.8500 0.3250 0.0980]}; % 橙色
    markers = {'o', 's', '^', 'd', 'v', '<', '>', 'p'};
    antenna_names = {'天线1', '天线2', '天线3', '天线4', '天线5', '天线6', '天线7', '天线8'};
    
    hold on;
    for ant = 1:8
        if ant <= size(calibration_data.i_float, 2)
            % 提取有效数据（非NaN值）
            valid_indices = ~isnan(calibration_data.i_float(:, ant)) & ~isnan(calibration_data.q_float(:, ant));
            if any(valid_indices)
                i_converted = calibration_data.i_float(valid_indices, ant);
                q_converted = calibration_data.q_float(valid_indices, ant);
                
                scatter(i_converted, q_converted, ...
                    50, colors{ant}, markers{ant}, 'filled', ...
                    'DisplayName', antenna_names{ant}, 'LineWidth', 1);
            end
        end
    end
    
    xlabel('I路数据 (转换后)', 'FontSize', 12);
    ylabel('Q路数据 (转换后)', 'FontSize', 12);
    title('各天线I-Q校准因子散点图 (18bit有符号数转换)', 'FontSize', 14, 'FontWeight', 'bold');
    legend('Location', 'best', 'FontSize', 10);
    grid on;
    grid minor;
    
    % 添加坐标轴
    ax = gca;
    ax.XAxisLocation = 'origin';
    ax.YAxisLocation = 'origin';
    
    % 固定坐标轴范围为-2到2
    xlim([-2, 2]);
    ylim([-2, 2]);
    
    hold off;
    
    % 保存图形
    saveas(gcf, 'calibration_iq_scatter_plot.png');
    fprintf('I-Q散点图已保存为: calibration_iq_scatter_plot.png\n');
end

function display_statistics(calibration_data)
    fprintf('\n=== 校准因子统计信息 ===\n');
    fprintf('校准次数: %d\n', size(calibration_data.i_float, 1));
    fprintf('通道数量: %d\n', size(calibration_data.i_float, 2));
    
    fprintf('\nI路校准因子统计:\n');
    fprintf('通道\t均值\t\t标准差\t\t最小值\t\t最大值\n');
    for ch = 1:8
        data = calibration_data.i_float(:, ch);
        fprintf('%d\t%.6f\t%.6f\t%.6f\t%.6f\n', ch-1, mean(data), std(data), min(data), max(data));
    end
    
    fprintf('\nQ路校准因子统计:\n');
    fprintf('通道\t均值\t\t标准差\t\t最小值\t\t最大值\n');
    for ch = 1:8
        data = calibration_data.q_float(:, ch);
        fprintf('%d\t%.6f\t%.6f\t%.6f\t%.6f\n', ch-1, mean(data), std(data), min(data), max(data));
    end
    
    fprintf('\n时间戳列表:\n');
    for i = 1:length(calibration_data.timestamps)
        fprintf('%d: %s\n', i, calibration_data.timestamps{i});
    end
end

